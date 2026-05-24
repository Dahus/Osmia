/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "Database/SqlStatement.h"

class ConcreteDatabase;

class SqlStatementImpl : public SqlStatement
{
public:
    SqlStatementImpl(const SqlStatementImpl&) = delete;
    SqlStatementImpl& operator=(const SqlStatementImpl&) = delete;

    bool execute()       override;
    bool directExecute() override;

    ~SqlStatementImpl() = default;

private:
    friend class ConcreteDatabase;

    SqlStatementImpl(const SqlStatementID& id, ConcreteDatabase& db)
        : _db(&db)
    {
        _stmtId = id;
        _params.reset(_stmtId.numArgs());
    }

    ConcreteDatabase* _db = nullptr;
};
