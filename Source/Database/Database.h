/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "QueryResult.h"
#include "SqlStatement.h"
#include "Callback.h"
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <functional>

class Database
{
public:
    virtual ~Database() = default;

    using KeyValueColl = std::map<std::string, std::string>;

    // Initialise connection pool
    virtual bool initialise(const KeyValueColl& connParams, size_t nConns = 1) = 0;

    // Start/stop async worker thread
    virtual void allowAsyncOperations() = 0;
    virtual void haltAsyncOperations() = 0;

    // Synchronous queries — caller formats SQL before passing
    virtual std::unique_ptr<QueryResult> query(const std::string& sql) = 0;

    // Synchronous execute (INSERT/UPDATE/DELETE)
    virtual bool execute(const std::string& sql) = 0;

    // Direct execute (bypasses async queue, waits for result)
    virtual bool directExecute(const std::string& sql) = 0;

    // Batch execute — multiple statements in one call
    virtual bool executeBatch(const std::vector<std::string>& statements) = 0;

    // Transactions
    virtual bool transactionStart() = 0;
    virtual bool transactionCommit() = 0;
    virtual bool transactionRollback() = 0;

    // Async query with callback
    virtual bool asyncQuery(QueryCallback::FuncType callback, const std::string& sql) = 0;
    virtual void invokeCallbacks() = 0;

    // Prepared statements
    virtual std::unique_ptr<SqlStatement> makeStatement(SqlStatementID& index, const std::string& sqlText) = 0;

    // String escaping
    virtual std::string escape(const std::string& str) const = 0;

    // Health check
    virtual bool checkConnections() = 0;

    // Is DB ready
    virtual explicit operator bool() const = 0;

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

protected:
    Database() = default;
};
