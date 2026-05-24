/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#include "SqlStatementImpl.h"
#include "ConcreteDatabase.h"

bool SqlStatementImpl::execute()
{
    SqlStmtParameters params;
    _params.swap(params);
    return _db->executeStmt(_stmtId, params);
}

bool SqlStatementImpl::directExecute()
{
    SqlStmtParameters params;
    _params.swap(params);
    return _db->directExecuteStmt(_stmtId, params);
}
