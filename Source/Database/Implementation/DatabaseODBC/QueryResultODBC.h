/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "Database/QueryResult.h"
#include "Database/Field.h"
#ifdef _WIN32
#   include <windows.h>
#endif
#include <sql.h>
#include <sqlext.h>
#include <vector>
#include <string>

class QueryResultODBC : public QueryResult
{
public:
    // Takes ownership of the statement handle
    explicit QueryResultODBC(SQLHSTMT hStmt);
    ~QueryResultODBC() override;

    bool            fetchRow()        override;
    const std::vector<Field>& fields() const override { return _fields; }
    size_t          numFields()       const override { return _numFields; }
    UInt64          numRows()         const override { return _numRows; }
    QueryFieldNames fetchFieldNames() const override { return _fieldNames; }

private:
    void bindColumns();

    SQLHSTMT                 _hStmt;
    size_t                   _numFields = 0;
    UInt64                   _numRows = 0;
    std::vector<Field>       _fields;
    QueryFieldNames          _fieldNames;

    // Storage for column data — each column gets a buffer
    struct ColBuffer
    {
        std::string  data;
        SQLLEN       indicator = 0;
    };
    std::vector<ColBuffer> _buffers;
};
