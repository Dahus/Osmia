/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#include "SqlPreparedStatement.h"
#include "SqlConnection.h"
#include "ConcreteDatabase.h"
#include "Shared/Server/Log/Logger.h"
#include <cstring>
#include <sstream>
#include <iomanip>

SqlPlainPreparedStatement::SqlPlainPreparedStatement(const std::string& sql, SqlConnection& conn)
    : SqlPreparedStatement(sql, conn) {}

void SqlPlainPreparedStatement::prepare()
{
    _numParams = std::count(_stmtSql.begin(), _stmtSql.end(), '?');
    _isQuery = _stmtSql.size() >= 6 &&
        _strnicmp("select", _stmtSql.c_str(), 6) == 0;
    _prepared = true;
}

void SqlPlainPreparedStatement::bind(const SqlStmtParameters& params)
{
    if (!isPrepared()) return;
    if (_numParams != params.boundParams())
    {
        Log::error("SqlPlainPreparedStatement: not all parameters bound");
        return;
    }
    _preparedSql = _stmtSql;
    size_t pos = 0;
    for (const auto& field : params.params())
    {
        pos = _preparedSql.find('?', pos);
        if (pos == std::string::npos) break;
        std::ostringstream fmt;
        fieldToString(field, fmt);
        std::string val = fmt.str();
        _preparedSql.replace(pos, 1, val);
        pos += val.length();
    }
}

bool SqlPlainPreparedStatement::execute()
{
    if (!isPrepared() || _preparedSql.empty()) return false;
    return _conn.execute(_preparedSql);
}

std::string SqlPlainPreparedStatement::getSqlString(bool withValues) const
{
    return withValues ? _preparedSql : SqlPreparedStatement::getSqlString(withValues);
}

void SqlPlainPreparedStatement::fieldToString(const SqlStmtField& field, std::ostringstream& out) const
{
    switch (field.type())
    {
    case SqlStmtField::FIELD_BOOL:   out << "'" << UInt32(field.toBool()) << "'"; break;
    case SqlStmtField::FIELD_UI8:    out << "'" << UInt32(field.toUint8()) << "'"; break;
    case SqlStmtField::FIELD_UI16:   out << "'" << UInt32(field.toUint16()) << "'"; break;
    case SqlStmtField::FIELD_UI32:   out << "'" << field.toUint32() << "'"; break;
    case SqlStmtField::FIELD_UI64:   out << "'" << field.toUint64() << "'"; break;
    case SqlStmtField::FIELD_I8:     out << "'" << Int32(field.toInt8()) << "'"; break;
    case SqlStmtField::FIELD_I16:    out << "'" << Int32(field.toInt16()) << "'"; break;
    case SqlStmtField::FIELD_I32:    out << "'" << field.toInt32() << "'"; break;
    case SqlStmtField::FIELD_I64:    out << "'" << field.toInt64() << "'"; break;
    case SqlStmtField::FIELD_FLOAT:  out << "'" << field.toFloat() << "'"; break;
    case SqlStmtField::FIELD_DOUBLE: out << "'" << field.toDouble() << "'"; break;
    case SqlStmtField::FIELD_STRING:
        out << "'" << _conn.getDB().escape(field.toString()) << "'";
        break;
    case SqlStmtField::FIELD_BINARY:
    {
        const auto* data = reinterpret_cast<const unsigned char*>(field.buff());
        size_t len = field.size();
        std::ostringstream hex;
        hex << "UNHEX('";
        for (size_t i = 0; i < len; i++)
            hex << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(data[i]);
        hex << "')";
        out << hex.str();
        break;
    }
    default: break;
    }
}
