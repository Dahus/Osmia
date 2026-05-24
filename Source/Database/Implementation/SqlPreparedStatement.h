/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "Shared/Common/Types.h"
#include <string>
#include <sstream>

class SqlConnection;
class SqlStmtField;
class SqlStmtParameters;

class SqlPreparedStatement
{
public:
    virtual ~SqlPreparedStatement() = default;

    bool   isPrepared()  const { return _prepared; }
    bool   isQuery()     const { return _isQuery; }
    size_t numParams()   const { return _numParams; }
    size_t numColumns()  const { return _isQuery ? _numColumns : 0; }

    virtual void        prepare() = 0;
    virtual void        bind(const SqlStmtParameters& params) = 0;
    virtual bool        execute() = 0;
    virtual int         lastError()      const { return 0; }
    virtual std::string lastErrorDescr() const { return {}; }

    virtual std::string getSqlString(bool withValues = false) const
    {
        return _stmtSql;
    }

protected:
    explicit SqlPreparedStatement(const std::string& sql, SqlConnection& conn)
        : _stmtSql(sql)
        , _conn(conn)
    {}

    size_t _numParams = 0;
    size_t _numColumns = 0;
    bool   _isQuery = false;
    bool   _prepared = false;

    std::string    _stmtSql;
    SqlConnection& _conn;
};

class SqlPlainPreparedStatement : public SqlPreparedStatement
{
public:
    SqlPlainPreparedStatement(const std::string& sql, SqlConnection& conn);
    ~SqlPlainPreparedStatement() = default;
    void        prepare()                             override;
    void        bind(const SqlStmtParameters& params) override;
    bool        execute()                             override;
    std::string getSqlString(bool withValues = false) const override;
protected:
    void fieldToString(const SqlStmtField& field, std::ostringstream& out) const;
    std::string _preparedSql;
};
