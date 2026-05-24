/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "Shared/Common/Types.h"
#include "Database/Callback.h"
#include "Database/SqlStatement.h"
#include <queue>
#include <mutex>
#include <vector>
#include <memory>
#include <functional>

class Database;
class SqlConnection;
class SqlStmtParameters;

// ---- BASE ----
class SqlOperation
{
public:
    virtual ~SqlOperation() = default;

    bool execute(SqlConnection& conn);

    SqlOperation(const SqlOperation&) = delete;
    SqlOperation& operator=(const SqlOperation&) = delete;

protected:
    SqlOperation() = default;
    friend class SqlTransaction;

    virtual bool rawExecute(SqlConnection& conn, bool throwExc = false) = 0;

    using SuccessCallback = std::function<void()>;
    virtual void transExecute(SqlConnection& conn, SuccessCallback& onSuccess)
    {
        rawExecute(conn, true);
    }
};

// ---- PLAIN REQUEST ----
class SqlPlainRequest : public SqlOperation
{
public:
    explicit SqlPlainRequest(std::string sql) : _sql(std::move(sql)) {}

protected:
    bool rawExecute(SqlConnection& conn, bool throwExc) override;

private:
    std::string _sql;
};

// ---- TRANSACTION ----
class SqlTransaction : public SqlOperation
{
public:
    SqlTransaction() = default;

    void queueOperation(std::unique_ptr<SqlOperation> op)
    {
        _queue.push_back(std::move(op));
    }

protected:
    bool rawExecute(SqlConnection& conn, bool throwExc) override;

private:
    std::vector<std::unique_ptr<SqlOperation>> _queue;
};

// ---- PREPARED REQUEST ----
class SqlPreparedRequest : public SqlOperation
{
public:
    SqlPreparedRequest(const SqlStatementID& id, SqlStmtParameters& params)
        : _id(id)
    {
        _params.swap(params);
    }

protected:
    bool rawExecute(SqlConnection& conn, bool throwExc) override;

private:
    SqlStatementID    _id;
    SqlStmtParameters _params;
};

// ---- RESULT QUEUE ----
class SqlResultQueue
{
public:
    void push(QueryCallback cb)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push(std::move(cb));
    }

    void processCallbacks();

    void clear()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        while (!_queue.empty()) _queue.pop();
    }

private:
    std::queue<QueryCallback> _queue;
    std::mutex                _mutex;
};

// ---- ASYNC QUERY ----
class SqlQuery : public SqlOperation
{
public:
    SqlQuery(std::string sql, QueryCallback callback, SqlResultQueue& queue)
        : _sql(std::move(sql))
        , _callback(std::move(callback))
        , _queue(&queue)
    {}

protected:
    bool rawExecute(SqlConnection& conn, bool throwExc) override;
    void transExecute(SqlConnection& conn, SuccessCallback& onSuccess) override;

private:
    std::string     _sql;
    QueryCallback   _callback;
    SqlResultQueue* _queue;
};
