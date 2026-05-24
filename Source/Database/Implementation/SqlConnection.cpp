/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#include "SqlConnection.h"
#include "ConcreteDatabase.h"
#include "SqlPreparedStatement.h"
#include "Shared/Server/Log/Logger.h"
#include <sstream>

 // SqlConnection
SqlPreparedStatement* SqlConnection::createPreparedStatement(const std::string& sql)
{
    return new SqlPlainPreparedStatement(sql, *this);
}

void SqlConnection::clear()
{
    Lock guard(*this);
    _stmtHolder.clear();
}

SqlPreparedStatement* SqlConnection::getStmt(const SqlStatementID& id)
{
    if (!id.isInitialized())
        return nullptr;

    UInt32 stmtId = id.getId();
    SqlPreparedStatement* stmt = _stmtHolder.get(stmtId);

    if (!stmt)
    {
        const char* sql = _dbEngine->getStmtString(stmtId);
        if (!sql || !sql[0]) { Log::error("SqlConnection: blank SQL statement string"); return nullptr; }
        stmt = createPreparedStatement(sql);
        stmt->prepare();
        _stmtHolder.insert(stmtId, stmt);
    }

    return stmt;
}

bool SqlConnection::executeStmt(const SqlStatementID& id, const SqlStmtParameters& params)
{
    if (!id.isInitialized())
        return false;

    SqlPreparedStatement* stmt = getStmt(id);
    stmt->bind(params);

    try
    {
        return stmt->execute();
    }
    catch (const SqlException& e)
    {
        if (e.isConnLost() || e.isRepeatable())
            _stmtHolder.release(id.getId());
        throw;
    }
}

size_t SqlConnection::escapeString(char* to, const char* from, size_t length) const
{
    strncpy(to, from, length);
    return length;
}

bool SqlConnection::transactionStart() { return false; }
bool SqlConnection::transactionCommit() { return false; }
bool SqlConnection::transactionRollback() { return false; }

// StmtHolder
void SqlConnection::StmtHolder::clear()
{
    _storage.clear();
}

SqlPreparedStatement* SqlConnection::StmtHolder::get(UInt32 id) const
{
    if (id < 1) return nullptr;
    size_t idx = id - 1;
    return idx < _storage.size() ? _storage[idx].get() : nullptr;
}

void SqlConnection::StmtHolder::insert(UInt32 id, SqlPreparedStatement* stmt)
{
    if (id < 1) { Log::error("SqlConnection: invalid stmt id"); return; }
    size_t idx = id - 1;
    if (idx >= _storage.size())
        _storage.resize(idx + 1);
    if (_storage[idx]) { Log::error("SqlConnection: stmt slot already occupied"); return; }
    _storage[idx].reset(stmt);
}

std::unique_ptr<SqlPreparedStatement> SqlConnection::StmtHolder::release(UInt32 id)
{
    if (id < 1) return nullptr;
    size_t idx = id - 1;
    if (idx >= _storage.size()) return nullptr;
    return std::move(_storage[idx]);
}
