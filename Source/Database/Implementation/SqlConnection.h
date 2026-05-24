/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "Shared/Common/Types.h"
#include "SqlPreparedStatement.h"
#include <stdexcept>
#include <mutex>
#include <memory>
#include <string>
#include <vector>
#include <sstream>

class ConcreteDatabase;
class QueryResult;
class SqlStatementID;
class SqlStmtParameters;

class SqlConnection
{
public:
    // Exception
    class SqlException : public std::runtime_error
    {
    public:
        SqlException(int code, const std::string& descr, const char* func,
            bool connLost = false, bool repeatable = false, std::string query = "")
            : std::runtime_error(descr)
            , _code(code)
            , _function(func)
            , _query(std::move(query))
            , _connLost(connLost)
            , _repeatable(repeatable)
        {}

        int                getCode()     const { return _code; }
        const char* getDescr()    const { return what(); }
        const char* getFunction() const { return _function; }
        const std::string& getQuery()    const { return _query; }
        bool               isConnLost()  const { return _connLost; }
        bool               isRepeatable()const { return _repeatable; }

        std::string toString() const
        {
            std::ostringstream oss;
            oss << "[" << _code << "] " << what()
                << " (in " << (_function ? _function : "?") << ")";
            if (!_query.empty())
                oss << " Query: " << _query;
            return oss.str();
        }

    private:
        int         _code;
        const char* _function;
        std::string _query;
        bool        _connLost;
        bool        _repeatable;
    };

    virtual ~SqlConnection() = default;

    virtual void connect() = 0;

    virtual std::unique_ptr<QueryResult> query(const std::string& sql) = 0;
    virtual bool                         execute(const std::string& sql) = 0;

    virtual size_t escapeString(char* to, const char* from, size_t length) const;

    virtual bool transactionStart();
    virtual bool transactionCommit();
    virtual bool transactionRollback();

    bool executeStmt(const SqlStatementID& id, const SqlStmtParameters& params);
    SqlPreparedStatement* getStmt(const SqlStatementID& id);

    ConcreteDatabase& getDB() { return *_dbEngine; }

    // RAII lock
    class Lock
    {
    public:
        explicit Lock(SqlConnection& conn) : _guard(conn._lock) {}
        ~Lock() = default;
        Lock(const Lock&) = delete;
        Lock& operator=(const Lock&) = delete;
    private:
        std::lock_guard<std::mutex> _guard;
    };

    SqlConnection(const SqlConnection&) = delete;
    SqlConnection& operator=(const SqlConnection&) = delete;

protected:
    explicit SqlConnection(ConcreteDatabase& db) : _dbEngine(&db) {}

    virtual SqlPreparedStatement* createPreparedStatement(const std::string& sql);
    virtual void clear();

    ConcreteDatabase* _dbEngine;

private:
    std::mutex _lock;

    class StmtHolder
    {
    public:
        void                  clear();
        SqlPreparedStatement* get(UInt32 id) const;
        void                  insert(UInt32 id, SqlPreparedStatement* stmt);
        std::unique_ptr<SqlPreparedStatement> release(UInt32 id);
    private:
        std::vector<std::unique_ptr<SqlPreparedStatement>> _storage;
    } _stmtHolder;
};
