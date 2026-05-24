/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "Database/QueryResult.h"
#include <vector>

class QueryResultImpl : public QueryResult
{
public:
    QueryResultImpl() : _numFields(0), _numRows(0) {}

    QueryResultImpl(UInt64 rowCount, size_t fieldCount)
        : _row(fieldCount)
        , _numFields(fieldCount)
        , _numRows(rowCount)
    {}

    ~QueryResultImpl() = default;

    const std::vector<Field>& fields()   const override { return _row; }
    size_t                    numFields() const override { return _numFields; }
    UInt64                    numRows()   const override { return _numRows; }

    // fetchRow() и fetchFieldNames() Ч реализует конкретна€ Ѕƒ (ODBC)

protected:
    void setNumFields(size_t n) { _numFields = n; }
    void setNumRows(UInt64 n) { _numRows = n; }

    std::vector<Field> _row;

private:
    size_t _numFields;
    UInt64 _numRows;
};
