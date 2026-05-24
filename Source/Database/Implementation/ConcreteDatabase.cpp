/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#include "ConcreteDatabase.h"
#include "Database/Callback.h"
#include "SqlOperations.h"
#include "SqlConnection.h"
#include "SqlStatementImpl.h"
#include "RetrySqlOp.h"
#include "Shared/Server/Log/Logger.h"


static const size_t MIN_POOL_SIZE = 1;
static const size_t MAX_POOL_SIZE = 16;

ConcreteDatabase::ConcreteDatabase() {}

ConcreteDatabase::~ConcreteDatabase()
{
    haltAsyncOperations();
    _resultQueue.clear();
    _asyncConn.reset();
    _queryConns.clear();
}

bool ConcreteDatabase::initialise(const KeyValueColl& connParams, size_t nConns)
{
    haltAsyncOperations();
    _resultQueue.clear();
    _asyncConn.reset();
    _queryConns.clear();

    size_t poolSize = std::max(MIN_POOL_SIZE, std::min(nConns, MAX_POOL_SIZE));
    _queryConns.reserve(poolSize);

    try
    {
        for (size_t i = 0; i < poolSize; i++)
        {
            Log::info("ConcreteDatabase: creating pool conn {}", i);
            auto conn = createConnection(connParams);
            conn->connect();
            _queryConns.push_back(std::move(conn));
        }
        Log::info("ConcreteDatabase: creating async conn");
        _asyncConn = createConnection(connParams);
        _asyncConn->connect();
    }
    catch (const SqlConnection::SqlException& e)
    {
        Log::error("ConcreteDatabase: connection failed — {}", e.what());
        return false;
    }

    // Start delay thread
    _delayRunner = std::make_unique<DelayThreadRunnable>(createDelayThread());
    _delayRunner->start();

    Log::info("ConcreteDatabase: initialised, pool={}", poolSize);
    return true;
}

void ConcreteDatabase::haltAsyncOperations()
{
    if (!_delayRunner) return;
    _delayRunner->stop();
    _delayRunner.reset();
}

std::unique_ptr<SqlDelayThread> ConcreteDatabase::createDelayThread()
{
    return std::make_unique<SqlDelayThread>(*this, *_asyncConn);
}

void ConcreteDatabase::DelayThreadRunnable::start()
{
    _thread = std::thread([this] { _body->run(); });
}

void ConcreteDatabase::DelayThreadRunnable::stop()
{
    _body->stop();
    if (_thread.joinable())
        _thread.join();
}

std::unique_ptr<QueryResult> ConcreteDatabase::query(const std::string& sql)
{
    SqlConnection& conn = getQueryConnection();
    SqlConnection::Lock guard(conn);
    return Retry::SqlOp<std::unique_ptr<QueryResult>>(
        [&sql](SqlConnection& c) { return c.query(sql); })(conn, "Query");
}

bool ConcreteDatabase::directExecute(const std::string& sql)
{
    if (!_asyncConn) return false;
    SqlConnection& conn = getAsyncConnection();
    SqlConnection::Lock guard(conn);
    return Retry::SqlOp<bool>(
        [&sql](SqlConnection& c) { return c.execute(sql); })(conn, "DirectExecute");
}

bool ConcreteDatabase::execute(const std::string& sql)
{
    if (!_asyncConn) return false;
    TransHelper& th = getTransHelper();
    if (th.get())
    {
        th.get()->queueOperation(std::make_unique<SqlPlainRequest>(sql));
        return true;
    }
    if (!_asyncAllowed.load())
        return directExecute(sql);
    return _delayRunner->queueOperation(std::make_unique<SqlPlainRequest>(sql));
}

bool ConcreteDatabase::executeBatch(const std::vector<std::string>& statements)
{
    if (!_asyncConn || statements.empty()) return false;
    if (!transactionStart()) return false;
    for (const auto& sql : statements)
        execute(sql);
    return transactionCommit();
}

bool ConcreteDatabase::transactionStart()
{
    if (!_asyncConn) return false;
    getTransHelper().init();
    return true;
}

bool ConcreteDatabase::transactionCommit()
{
    TransHelper& th = getTransHelper();
    if (!_asyncConn || !th.get()) return false;
    if (!_asyncAllowed.load())
    {
        std::unique_ptr<SqlTransaction> pTrans(th.detach());
        SqlConnection& conn = getAsyncConnection();
        SqlConnection::Lock guard(conn);
        return pTrans->execute(conn);
    }
    _delayRunner->queueOperation(std::unique_ptr<SqlOperation>(th.detach()));
    return true;
}

bool ConcreteDatabase::transactionRollback()
{
    getTransHelper().reset();
    return true;
}

bool ConcreteDatabase::asyncQuery(QueryCallback::FuncType func, const std::string& sql)
{
    if (sql.empty()) return false;
    QueryCallback cb(std::move(func));
    
    return doDelay(sql, std::move(cb));
}

bool ConcreteDatabase::doDelay(const std::string& sql, QueryCallback&& callback)
{
    return _delayRunner->queueOperation(std::make_unique<SqlQuery>(sql, std::move(callback), _resultQueue));
}

void ConcreteDatabase::invokeCallbacks()
{
    _resultQueue.processCallbacks();
}

std::unique_ptr<SqlStatement> ConcreteDatabase::makeStatement(SqlStatementID& index, const std::string& sqlText)
{
    if (!index.isInitialized())
    {
        size_t nParams = std::count(sqlText.begin(), sqlText.end(), '?');
        UInt32 nId = _prepStmtRegistry.getStmtId(sqlText);
        index.init(nId, nParams);
    }
    else if (!_prepStmtRegistry.idDefined(index.getId()))
        _prepStmtRegistry.insertStmt(index.getId(), sqlText);

    return std::unique_ptr<SqlStatementImpl>(new SqlStatementImpl(index, *this));
}

const char* ConcreteDatabase::getStmtString(UInt32 stmtId) const
{
    return _prepStmtRegistry.getStmtString(stmtId);
}

std::string ConcreteDatabase::escape(const std::string& str) const
{
    if (str.empty() || _queryConns.empty()) return str;
    std::vector<char> buf(str.size() * 2 + 1);
    size_t len = _queryConns[0]->escapeString(buf.data(), str.c_str(), str.size());
    return len > 0 ? std::string(buf.data(), len) : str;
}

bool ConcreteDatabase::checkConnections()
{
    const std::string sql = "SELECT 1";
    auto check = [](QueryResult* res) {
        return res && res->fetchRow() && res->at(0).getInt32() == 1;
    };

    {
        SqlConnection& conn = getAsyncConnection();
        SqlConnection::Lock guard(conn);
        auto res = conn.query(sql);
        if (!check(res.get())) return false;
    }

    for (auto& conn : _queryConns)
    {
        SqlConnection::Lock guard(*conn);
        auto res = conn->query(sql);
        if (!check(res.get())) return false;
    }

    return true;
}

SqlConnection& ConcreteDatabase::getQueryConnection()
{
    return *_queryConns[_currConn++ % _queryConns.size()];
}

SqlConnection& ConcreteDatabase::getAsyncConnection()
{
    return *_asyncConn;
}

bool ConcreteDatabase::executeStmt(const SqlStatementID& id, SqlStmtParameters& params)
{
    if (!_asyncConn) return false;
    TransHelper& th = getTransHelper();
    if (th.get())
    {
        th.get()->queueOperation(std::make_unique<SqlPreparedRequest>(id, params));
        return true;
    }
    if (!_asyncAllowed.load())
        return directExecuteStmt(id, params);
    return _delayRunner->queueOperation(std::make_unique<SqlPreparedRequest>(id, params));
}

bool ConcreteDatabase::directExecuteStmt(const SqlStatementID& id, SqlStmtParameters& params)
{
    SqlConnection& conn = getAsyncConnection();
    SqlConnection::Lock guard(conn);
    return conn.executeStmt(id, params);
}

// TransHelper
SqlTransaction* ConcreteDatabase::TransHelper::init()
{
    _trans = std::make_unique<SqlTransaction>();
    return get();
}

ConcreteDatabase::TransHelper& ConcreteDatabase::getTransHelper()
{
    thread_local TransHelper th;
    return th;
}

// PreparedStmtRegistry
void ConcreteDatabase::PreparedStmtRegistry::insertStmt(UInt32 id, std::string fmt)
{
    std::lock_guard<std::mutex> guard(_lock);
    _idMap[id] = fmt;
    _stringMap[fmt] = id;
}

UInt32 ConcreteDatabase::PreparedStmtRegistry::getStmtId(const std::string& fmt)
{
    std::lock_guard<std::mutex> guard(_lock);
    auto it = _stringMap.find(fmt);
    if (it != _stringMap.end()) return it->second;
    UInt32 nId = ++_nextId;
    _idMap[nId] = fmt;
    _stringMap[fmt] = nId;
    return nId;
}

const char* ConcreteDatabase::PreparedStmtRegistry::getStmtString(UInt32 id) const
{
    std::lock_guard<std::mutex> guard(_lock);
    auto it = _idMap.find(id);
    return it != _idMap.end() ? it->second.c_str() : nullptr;
}

bool ConcreteDatabase::PreparedStmtRegistry::idDefined(UInt32 id) const
{
    std::lock_guard<std::mutex> guard(_lock);
    return _idMap.count(id) > 0;
}
