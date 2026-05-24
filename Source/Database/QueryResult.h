/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "Shared/Common/Types.h"
#include "Field.h"
#include <vector>
#include <string>
#include <stdexcept>

using QueryFieldNames = std::vector<std::string>;

class QueryResult
{
public:
    virtual ~QueryResult() = default;

    // Fetch next row, returns false when no more rows
    virtual bool fetchRow() = 0;

    // Access fields of current row
    virtual const std::vector<Field>& fields() const = 0;

    const Field& at(size_t idx) const
    {
        const auto& f = fields();
        if (idx >= f.size())
            throw std::out_of_range("Field index out of range: " + std::to_string(idx));
        return f[idx];
    }

    const Field& operator[](size_t idx) const { return at(idx); }

    virtual size_t numFields() const = 0;
    virtual UInt64 numRows()   const = 0;

    virtual QueryFieldNames fetchFieldNames() const = 0;

    QueryResult(const QueryResult&) = delete;
    QueryResult& operator=(const QueryResult&) = delete;

protected:
    QueryResult() = default;
};
