/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#include "QueryResultODBC.h"
#include <stdexcept>

QueryResultODBC::QueryResultODBC(SQLHSTMT hStmt)
    : _hStmt(hStmt)
{
    SQLSMALLINT cols = 0;
    SQLNumResultCols(_hStmt, &cols);
    _numFields = static_cast<size_t>(cols);

    // Get row count (may be -1 for SELECT)
    SQLLEN rows = 0;
    SQLRowCount(_hStmt, &rows);
    _numRows = rows > 0 ? static_cast<UInt64>(rows) : 0;

    _fields.resize(_numFields);
    _buffers.resize(_numFields);
    _fieldNames.resize(_numFields);

    bindColumns();
}

QueryResultODBC::~QueryResultODBC()
{
    if (_hStmt != SQL_NULL_HSTMT)
    {
        SQLFreeStmt(_hStmt, SQL_CLOSE);
        SQLFreeHandle(SQL_HANDLE_STMT, _hStmt);
        _hStmt = SQL_NULL_HSTMT;
    }
}

void QueryResultODBC::bindColumns()
{
    for (size_t i = 0; i < _numFields; i++)
    {
        SQLSMALLINT colNum = static_cast<SQLSMALLINT>(i + 1);

        // Get column name
        SQLCHAR colName[256] = {};
        SQLSMALLINT nameLen = 0;
        SQLSMALLINT dataType = 0;
        SQLULEN colSize = 0;
        SQLSMALLINT decDigits = 0;
        SQLSMALLINT nullable = 0;

        SQLDescribeColA(_hStmt, colNum,
            colName, sizeof(colName), &nameLen,
            &dataType, &colSize, &decDigits, &nullable);

        _fieldNames[i] = reinterpret_cast<char*>(colName);

        // Allocate buffer — max 8KB per column
        _buffers[i].data.resize(8192);

        SQLBindCol(_hStmt, colNum, SQL_C_CHAR,
            _buffers[i].data.data(),
            static_cast<SQLLEN>(_buffers[i].data.size()),
            &_buffers[i].indicator);
    }
}

bool QueryResultODBC::fetchRow()
{
    SQLRETURN ret = SQLFetch(_hStmt);
    if (ret == SQL_NO_DATA || ret == SQL_ERROR)
        return false;

    for (size_t i = 0; i < _numFields; i++)
    {
        if (_buffers[i].indicator == SQL_NULL_DATA)
            _fields[i].setValue(nullptr);
        else
            _fields[i].setValue(_buffers[i].data.c_str());
    }

    return true;
}
