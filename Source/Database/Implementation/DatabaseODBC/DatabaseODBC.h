/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "../ConcreteDatabase.h"
#include "../SqlConnection.h"
#ifdef _WIN32
#   include <windows.h>
#endif
#include <sql.h>
#include <sqlext.h>
#include <string>
#include <memory>

 // ---- ODBC Connection ----
class ODBCConnection : public SqlConnection
{
public:
    ODBCConnection(ConcreteDatabase& db, const Database::KeyValueColl& connParams);
    ~ODBCConnection();

    void connect() override;

    std::unique_ptr<QueryResult> query(const std::string& sql)  override;
    bool                         execute(const std::string& sql) override;

    size_t escapeString(char* to, const char* from, size_t length) const override;

    bool transactionStart()    override;
    bool transactionCommit()   override;
    bool transactionRollback() override;

    ODBCConnection(const ODBCConnection&) = delete;
    ODBCConnection& operator=(const ODBCConnection&) = delete;

private:
    bool        isConnectionLost() const;
    std::string lastErrorDescr(SQLHANDLE handle, SQLSMALLINT handleType) const;

    // ODBC handles
    SQLHENV _hEnv = SQL_NULL_HENV;
    SQLHDBC _hDbc = SQL_NULL_HDBC;

    // Connection string built from connParams
    std::string _connStr;
};

// ---- ODBC Database ----
class DatabaseODBC : public ConcreteDatabase
{
public:
    DatabaseODBC();
    ~DatabaseODBC();

    DatabaseODBC(const DatabaseODBC&) = delete;
    DatabaseODBC& operator=(const DatabaseODBC&) = delete;

protected:
    std::unique_ptr<SqlConnection> createConnection(const KeyValueColl& connParams) override;

private:
    static size_t _dbCount;
};
