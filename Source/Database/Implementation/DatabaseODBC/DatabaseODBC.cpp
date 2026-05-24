/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#include "DatabaseODBC.h"
#include "QueryResultODBC.h"
#include "Shared/Server/Log/Logger.h"
#include <stdexcept>

 // ---- ODBCConnection ----

ODBCConnection::ODBCConnection(ConcreteDatabase& db, const Database::KeyValueColl& connParams)
    : SqlConnection(db)
{
    auto it = connParams.find("connectionstring");
    if (it != connParams.end() && !it->second.empty())
    {
        _connStr = it->second;
        return;
    }

    auto get = [&](const std::string& key) -> const std::string& {
        auto i = connParams.find(key);
        if (i == connParams.end() || i->second.empty())
            throw SqlConnection::SqlException(0, "ODBC: missing '" + key + "' in config", __FUNCTION__);
        return i->second;
    };

    const std::string& driver = get("type");
    const std::string& host = get("host");
    const std::string& port = get("port");
    const std::string& dbname = get("database");
    const std::string& user = get("username");
    const std::string& pass = get("password");

    _connStr = "DRIVER={" + driver + "};SERVER=" + host + ";PORT=" + port +
        ";DATABASE=" + dbname + ";UID=" + user + ";PWD=" + pass + ";";
}

ODBCConnection::~ODBCConnection()
{
    if (_hDbc != SQL_NULL_HDBC)
    {
        SQLDisconnect(_hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, _hDbc);
    }
    if (_hEnv != SQL_NULL_HENV)
        SQLFreeHandle(SQL_HANDLE_ENV, _hEnv);
}

void ODBCConnection::connect()
{
    Log::debug("ODBC: connecting, connStr={}", _connStr);

    if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_hEnv) != SQL_SUCCESS)
        throw SqlConnection::SqlException(0, "ODBC: failed to allocate environment handle", __FUNCTION__);

    SQLSetEnvAttr(_hEnv, SQL_ATTR_ODBC_VERSION,
        reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);

    if (SQLAllocHandle(SQL_HANDLE_DBC, _hEnv, &_hDbc) != SQL_SUCCESS)
        throw SqlConnection::SqlException(0, "ODBC: failed to allocate connection handle", __FUNCTION__);

    SQLRETURN ret = SQLDriverConnectA(
        _hDbc, nullptr,
        reinterpret_cast<SQLCHAR*>(_connStr.data()),
        static_cast<SQLSMALLINT>(_connStr.size()),
        nullptr, 0, nullptr,
        SQL_DRIVER_NOPROMPT);

    Log::debug("ODBC: SQLDriverConnect ret={}", static_cast<int>(ret));

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
    {
        std::string err = lastErrorDescr(_hDbc, SQL_HANDLE_DBC);
        if (err.find("IM002") != std::string::npos || err.find("Data source name not found") != std::string::npos)
            throw SqlConnection::SqlException(0,
                "ODBC driver not found. Please install the ODBC driver for your database. ",
                __FUNCTION__);
        throw SqlConnection::SqlException(0, "ODBC: connection failed — " + err, __FUNCTION__);
    }

    Log::info("ODBC: connected successfully");
}

std::unique_ptr<QueryResult> ODBCConnection::query(const std::string& sql)
{
    SQLHSTMT hStmt = SQL_NULL_HSTMT;
    SQLAllocHandle(SQL_HANDLE_STMT, _hDbc, &hStmt);

    SQLRETURN ret = SQLExecDirectA(hStmt,
        reinterpret_cast<SQLCHAR*>(const_cast<char*>(sql.c_str())),
        static_cast<SQLINTEGER>(sql.size()));

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
    {
        std::string err = lastErrorDescr(hStmt, SQL_HANDLE_STMT);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        throw SqlConnection::SqlException(0, "ODBC query failed: " + err, __FUNCTION__);
    }

    Log::trace("ODBC: query OK, sql={}", sql);

    return std::make_unique<QueryResultODBC>(hStmt);
}

bool ODBCConnection::execute(const std::string& sql)
{
    SQLHSTMT hStmt = SQL_NULL_HSTMT;
    SQLAllocHandle(SQL_HANDLE_STMT, _hDbc, &hStmt);

    SQLRETURN ret = SQLExecDirectA(hStmt,
        reinterpret_cast<SQLCHAR*>(const_cast<char*>(sql.c_str())),
        static_cast<SQLINTEGER>(sql.size()));

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO && ret != SQL_NO_DATA)
    {
        Log::error("ODBC execute failed: {}", lastErrorDescr(_hDbc, SQL_HANDLE_DBC));
        return false;
    }

    Log::trace("ODBC: execute OK, sql={}", sql);

    return true;
}

size_t ODBCConnection::escapeString(char* to, const char* from, size_t length) const
{
    size_t out = 0;
    for (size_t i = 0; i < length; i++)
    {
        if (from[i] == '\'')
            to[out++] = '\'';
        to[out++] = from[i];
    }
    to[out] = '\0';
    return out;
}

bool ODBCConnection::transactionStart()
{
    SQLSetConnectAttr(_hDbc, SQL_ATTR_AUTOCOMMIT,
        reinterpret_cast<SQLPOINTER>(SQL_AUTOCOMMIT_OFF), 0);

    Log::trace("ODBC: transaction start");

    return true;
}

bool ODBCConnection::transactionCommit()
{
    SQLRETURN ret = SQLEndTran(SQL_HANDLE_DBC, _hDbc, SQL_COMMIT);
    SQLSetConnectAttr(_hDbc, SQL_ATTR_AUTOCOMMIT,
        reinterpret_cast<SQLPOINTER>(SQL_AUTOCOMMIT_ON), 0);

    Log::trace("ODBC: transaction commit, ret={}", static_cast<int>(ret));

    return ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO;
}

bool ODBCConnection::transactionRollback()
{
    SQLRETURN ret = SQLEndTran(SQL_HANDLE_DBC, _hDbc, SQL_ROLLBACK);
    SQLSetConnectAttr(_hDbc, SQL_ATTR_AUTOCOMMIT,
        reinterpret_cast<SQLPOINTER>(SQL_AUTOCOMMIT_ON), 0);

    Log::trace("ODBC: transaction rollback, ret={}", static_cast<int>(ret));

    return ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO;
}

bool ODBCConnection::isConnectionLost() const
{
    SQLHSTMT hStmt = SQL_NULL_HSTMT;
    SQLAllocHandle(SQL_HANDLE_STMT, _hDbc, &hStmt);
    SQLRETURN ret = SQLExecDirectA(hStmt,
        reinterpret_cast<SQLCHAR*>(const_cast<char*>("SELECT 1")),
        SQL_NTS);
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
        Log::warning("ODBC: connection lost detected");

    return ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO;
}

std::string ODBCConnection::lastErrorDescr(SQLHANDLE handle, SQLSMALLINT handleType) const
{
    SQLCHAR state[6] = {};
    SQLINTEGER native = 0;
    SQLCHAR msg[512] = {};
    SQLSMALLINT len = 0;

    SQLGetDiagRecA(handleType, handle, 1, state, &native, msg, sizeof(msg), &len);

    return std::string(reinterpret_cast<char*>(state)) + ": " +
        std::string(reinterpret_cast<char*>(msg), len);
}

// ---- DatabaseODBC ----

size_t DatabaseODBC::_dbCount = 0;

DatabaseODBC::DatabaseODBC() { ++_dbCount; }
DatabaseODBC::~DatabaseODBC() { --_dbCount; }

std::unique_ptr<SqlConnection> DatabaseODBC::createConnection(const KeyValueColl& connParams)
{
    return std::make_unique<ODBCConnection>(*this, connParams);
}

// ---- Export ----
#ifdef _WIN32
#   define DB_EXPORT __declspec(dllexport)
#else
#   define DB_EXPORT __attribute__((visibility("default")))
#endif

extern "C" DB_EXPORT Database* createInstance()
{
    return new DatabaseODBC();
}
extern "C" DB_EXPORT void destroyInstance(Database* p)
{
    delete p;
}
