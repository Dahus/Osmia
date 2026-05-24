/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "Database/Database.h"
#include "Database/SqlStatement.h"
#include "SqlDelayThread.h"
#include "SqlOperations.h"
#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <map>
#include <vector>
#include <memory>
#include <string>

class ConcreteDatabase : public Database
{
public:
    virtual ~ConcreteDatabase();

    bool initialise(const KeyValueColl& connParams, size_t nConns = 1) override;

    void allowAsyncOperations() override { _asyncAllowed.store(true); }
    void haltAsyncOperations()  override;

    // Sync queries
    std::unique_ptr<QueryResult> query(const std::string& sql) override;

    // Sync execute
    bool execute(const std::string& sql)       override;
    bool directExecute(const std::string& sql) override;

    // Batch execute
    bool executeBatch(const std::vector<std::string>& statements) override;

    // Transactions
    bool transactionStart()    override;
    bool transactionCommit()   override;
    bool transactionRollback() override;

    // Async
    bool asyncQuery(QueryCallback::FuncType callback, const std::string& sql) override;
    void invokeCallbacks() override;

    // Prepared statements
    std::unique_ptr<SqlStatement> makeStatement(SqlStatementID& index, const std::string& sqlText) override;
    const char* getStmtString(UInt32 stmtId) const;

    std::string escape(const std::string& str) const override;
    bool checkConnections() override;

    explicit operator bool() const override
    {
        return !_queryConns.empty() && _asyncConn != nullptr;
    }

protected:
    ConcreteDatabase();

    bool doDelay(const std::string& sql, QueryCallback&& callback);

    virtual std::unique_ptr<SqlConnection>   createConnection(const KeyValueColl& connParams) = 0;
    virtual std::unique_ptr<SqlDelayThread>  createDelayThread();

    // Per-thread transaction helper
    class TransHelper
    {
    public:
        TransHelper() = default;
        SqlTransaction* init();
        SqlTransaction* get()    const { return _trans.get(); }
        SqlTransaction* detach() { return _trans.release(); }
        void            reset() { _trans.reset(); }
    private:
        std::unique_ptr<SqlTransaction> _trans;
    };

    // thread_local storage per ConcreteDatabase instance
    TransHelper& getTransHelper();

    SqlConnection& getQueryConnection();
    SqlConnection& getAsyncConnection();

    friend class SqlStatementImpl;
    bool executeStmt(const SqlStatementID& id, SqlStmtParameters& params);
    bool directExecuteStmt(const SqlStatementID& id, SqlStmtParameters& params);

    // Round-robin connection pool index
    std::atomic<uint32_t> _currConn{ 0 };
    std::atomic<bool>     _asyncAllowed{ false };

    // Connection pool
    std::vector<std::unique_ptr<SqlConnection>> _queryConns;
    std::unique_ptr<SqlConnection>              _asyncConn;

    SqlResultQueue _resultQueue;

    // Delay thread
    class DelayThreadRunnable
    {
    public:
        explicit DelayThreadRunnable(std::unique_ptr<SqlDelayThread> body)
            : _body(std::move(body)) {}

        void start();
        void stop();
        bool queueOperation(std::unique_ptr<SqlOperation> op)
        {
            return _body->queueOperation(std::move(op));
        }

    private:
        std::unique_ptr<SqlDelayThread> _body;
        std::thread                     _thread;
    };
    std::unique_ptr<DelayThreadRunnable> _delayRunner;

    // Prepared statement registry
    class PreparedStmtRegistry
    {
    public:
        PreparedStmtRegistry() : _nextId(0) {}

        void        insertStmt(UInt32 id, std::string fmt);
        UInt32      getStmtId(const std::string& fmt);
        const char* getStmtString(UInt32 id) const;
        bool        idDefined(UInt32 id) const;

    private:
        mutable std::mutex                      _lock;
        std::unordered_map<std::string, UInt32> _stringMap;
        std::map<UInt32, std::string>           _idMap;
        UInt32                                  _nextId;
    } _prepStmtRegistry;

    ConcreteDatabase(const ConcreteDatabase&) = delete;
    ConcreteDatabase& operator=(const ConcreteDatabase&) = delete;
};
