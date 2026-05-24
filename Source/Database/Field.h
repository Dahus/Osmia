/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "Shared/Common/Types.h"
#include <string>
#include <stdexcept>

class Field
{
public:
    enum DataTypes
    {
        DB_TYPE_UNKNOWN = 0x00,
        DB_TYPE_STRING = 0x01,
        DB_TYPE_INTEGER = 0x02,
        DB_TYPE_FLOAT = 0x03,
        DB_TYPE_BOOL = 0x04
    };

    Field() : _value(nullptr), _type(DB_TYPE_UNKNOWN) {}
    Field(const char* value, DataTypes type) : _value(value), _type(type) {}
    ~Field() = default;

    DataTypes   getType()   const { return _type; }
    bool        isNull()    const { return _value == nullptr; }
    const char* getCStr()   const { return _value; }
    std::string getString() const { return _value ? _value : ""; }

    double getDouble() const
    {
        if (!_value) return 0.0;
        try { return std::stod(_value); }
        catch (...) { return 0.0; }
    }

    float getFloat() const { return static_cast<float>(getDouble()); }

    bool getBool() const
    {
        if (!_value) return false;
        try { return std::stol(_value) > 0; }
        catch (...) { return false; }
    }

    Int8   getInt8()   const { return static_cast<Int8>(getLong()); }
    Int16  getInt16()  const { return static_cast<Int16>(getLong()); }
    Int32  getInt32()  const { return static_cast<Int32>(getLong()); }
    UInt8  getUInt8()  const { return static_cast<UInt8>(getULong()); }
    UInt16 getUInt16() const { return static_cast<UInt16>(getULong()); }
    UInt32 getUInt32() const { return static_cast<UInt32>(getULong()); }
    UInt64 getUInt64() const { return getULong(); }

    void setType(DataTypes type) { _type = type; }
    void setValue(const char* value) { _value = value; }

private:
    const char* _value;
    DataTypes   _type;

    long getLong() const
    {
        if (!_value) return 0;
        try { return std::stol(_value); }
        catch (...) { return 0; }
    }

    unsigned long long getULong() const
    {
        if (!_value) return 0;
        try { return std::stoull(_value); }
        catch (...) { return 0; }
    }
};
