/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#include "SqlOperations.h"
#include "SqlConnection.h"
#include "ConcreteDatabase.h"
#include "SqlPreparedStatement.h"
#include "RetrySqlOp.h"
#include "Shared/Server/Log/Logger.h"

 // ---- BASE ----
bool SqlOperation::execute(SqlConnection& conn)
{
    SqlConnection::Lock guard(conn);
    return rawExecute(conn);
}

// ---- PLAIN REQUEST ----
bool SqlPlainRequest::rawExecute(SqlConnection& conn, bool throwExc)
{
    return Retry::SqlOp<bool>(
        [&](SqlConnection& c) { return c.execute(_sql); }, throwExc)
        (conn, "PlainRequest", [&]() { return _sql; });
}

// ---- TRANSACTION ----
bool SqlTransaction::rawExecute(SqlConnection& conn, bool throwExc)
{
    if (_queue.empty()) return true;
    for (;;)
    {
        try
        {
            if (!conn.transactionStart())
                return false;
            std::vector<SuccessCallback> onSuccess;
            for (auto& op : _queue)
            {
                SuccessCallback cb;
                op->transExecute(conn, cb);
                if (cb) onSuccess.push_back(std::move(cb));
            }
            conn.transactionCommit();
            for (auto& cb : onSuccess)
                cb();
            break;
        }
        catch (const SqlConnection::SqlException& e)
        {
            if (!e.isConnLost())
            {
                try { conn.transactionRollback(); }
                catch (const SqlConnection::SqlException&) {}
            }
            else
            {
                try { conn.connect(); }
                catch (const SqlConnection::SqlException& ce)
                {
                    Log::error("SqlTransaction: connection error — {}", ce.what());
                    if (throwExc) throw;
                    return false;
                }
            }
            if (throwExc) throw;
            if (e.isRepeatable()) continue;
            return false;
        }
    }
    return true;
}

// ---- PREPARED REQUEST ----
bool SqlPreparedRequest::rawExecute(SqlConnection& conn, bool throwExc)
{
    return Retry::SqlOp<bool>(
        [&](SqlConnection& c) { return c.executeStmt(_id, _params); }, throwExc)
        (conn, "PreparedRequest");
}

// ---- RESULT QUEUE ----
void SqlResultQueue::processCallbacks()
{
    while (true)
    {
        QueryCallback cb;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_queue.empty()) break;
            cb = std::move(_queue.front());
            _queue.pop();
        }
        cb.invoke();
    }
}

// ---- ASYNC QUERY ----
bool SqlQuery::rawExecute(SqlConnection& conn, bool throwExc)
{
    if (!_queue) return false;
    auto res = Retry::SqlOp<std::unique_ptr<QueryResult>>(
        [&](SqlConnection& c) { return c.query(_sql); }, throwExc)
        (conn, "AsyncQuery", [&]() { return _sql; });
    _callback.setResult(res.release());
    if (!throwExc)
        _queue->push(std::move(_callback));
    return true;
}

void SqlQuery::transExecute(SqlConnection& conn, SuccessCallback& onSuccess)
{
    SqlOperation::transExecute(conn, onSuccess);
    onSuccess = [&]() { _queue->push(std::move(_callback)); };
}
