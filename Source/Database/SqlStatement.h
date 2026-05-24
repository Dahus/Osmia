/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "Shared/Common/Types.h"
#include <variant>
#include <vector>
#include <string>
#include <sstream>

class SqlStmtField
{
public:
    enum Type
    {
        FIELD_BOOL,
        FIELD_UI8, FIELD_UI16, FIELD_UI32, FIELD_UI64,
        FIELD_I8, FIELD_I16, FIELD_I32, FIELD_I64,
        FIELD_FLOAT, FIELD_DOUBLE,
        FIELD_STRING, FIELD_BINARY,
        FIELD_COUNT
    };

    template<typename T>
    explicit SqlStmtField(T param) { set(param); }
    SqlStmtField(const UInt8* data, size_t size) { set(data, size); }
    SqlStmtField(const char* str, size_t size) { set(str, size); }

    template<typename T> void set(T val) { _data = val; }
    void set(std::string val) { _data = std::move(val); }
    void set(ByteVector  val) { _data = std::move(val); }
    void set(const UInt8* data, size_t size)
    {
        ByteVector v(size);
        if (size > 0) std::copy(data, data + size, v.begin());
        _data = std::move(v);
    }
    void set(const char* str, size_t size)
    {
        _data = size > 0 ? std::string(str, size) : std::string{};
    }

    bool               toBool()   const { return std::get<bool>(_data); }
    UInt8              toUint8()  const { return std::get<UInt8>(_data); }
    Int8               toInt8()   const { return std::get<Int8>(_data); }
    UInt16             toUint16() const { return std::get<UInt16>(_data); }
    Int16              toInt16()  const { return std::get<Int16>(_data); }
    UInt32             toUint32() const { return std::get<UInt32>(_data); }
    Int32              toInt32()  const { return std::get<Int32>(_data); }
    UInt64             toUint64() const { return std::get<UInt64>(_data); }
    Int64              toInt64()  const { return std::get<Int64>(_data); }
    float              toFloat()  const { return std::get<float>(_data); }
    double             toDouble() const { return std::get<double>(_data); }
    const std::string& toString() const { return std::get<std::string>(_data); }
    const char* toCStr()   const { return toString().c_str(); }
    const ByteVector& toVector() const { return std::get<ByteVector>(_data); }

    Type type() const
    {
        return std::visit([](auto&& v) -> Type {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, bool>)        return FIELD_BOOL;
            if constexpr (std::is_same_v<T, UInt8>)       return FIELD_UI8;
            if constexpr (std::is_same_v<T, Int8>)        return FIELD_I8;
            if constexpr (std::is_same_v<T, UInt16>)      return FIELD_UI16;
            if constexpr (std::is_same_v<T, Int16>)       return FIELD_I16;
            if constexpr (std::is_same_v<T, UInt32>)      return FIELD_UI32;
            if constexpr (std::is_same_v<T, Int32>)       return FIELD_I32;
            if constexpr (std::is_same_v<T, UInt64>)      return FIELD_UI64;
            if constexpr (std::is_same_v<T, Int64>)       return FIELD_I64;
            if constexpr (std::is_same_v<T, float>)       return FIELD_FLOAT;
            if constexpr (std::is_same_v<T, double>)      return FIELD_DOUBLE;
            if constexpr (std::is_same_v<T, std::string>) return FIELD_STRING;
            if constexpr (std::is_same_v<T, ByteVector>)  return FIELD_BINARY;
            return FIELD_COUNT;
        }, _data);
    }

    const void* buff() const
    {
        return std::visit([](auto&& v) -> const void* {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::string>)
                return v.c_str();
            if constexpr (std::is_same_v<T, ByteVector>)
                return v.empty() ? nullptr : v.data();
            return static_cast<const void*>(&v);
        }, _data);
    }

    size_t size() const
    {
        return std::visit([](auto&& v) -> size_t {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::string>) return v.length();
            if constexpr (std::is_same_v<T, ByteVector>)  return v.size();
            return sizeof(v);
        }, _data);
    }

private:
    using FieldVariant = std::variant<
        bool,
        UInt8, UInt16, UInt32, UInt64,
        Int8, Int16, Int32, Int64,
        float, double,
        std::string, ByteVector
    > ;

    FieldVariant _data;
};

// Explicit specializations
template<> inline void SqlStmtField::set(bool       val) { _data = val; }
template<> inline void SqlStmtField::set(UInt8      val) { _data = val; }
template<> inline void SqlStmtField::set(Int8       val) { _data = val; }
template<> inline void SqlStmtField::set(UInt16     val) { _data = val; }
template<> inline void SqlStmtField::set(Int16      val) { _data = val; }
template<> inline void SqlStmtField::set(UInt32     val) { _data = val; }
template<> inline void SqlStmtField::set(Int32      val) { _data = val; }
template<> inline void SqlStmtField::set(UInt64     val) { _data = val; }
template<> inline void SqlStmtField::set(Int64      val) { _data = val; }
template<> inline void SqlStmtField::set(float      val) { _data = val; }
template<> inline void SqlStmtField::set(double     val) { _data = val; }
template<> inline void SqlStmtField::set(const char* val) { _data = std::string(val); }

// SqlStmtParameters
class SqlStmtParameters
{
public:
    using ParameterContainer = std::vector<SqlStmtField>;

    void   reserve(size_t n) { if (n > _params.capacity()) _params.reserve(n); }
    size_t boundParams()             const { return _params.size(); }
    void   addParam(SqlStmtField data) { _params.push_back(std::move(data)); }
    void   reset(size_t n = 0) { _params.clear(); if (n > 0) _params.reserve(n); }
    void   swap(SqlStmtParameters& other) { std::swap(_params, other._params); }
    const  ParameterContainer& params() const { return _params; }

private:
    ParameterContainer _params;
};

// SqlStatementID
class SqlStatementID
{
public:
    SqlStatementID() : _id(0), _numArgs(0), _initialized(false) {}

    UInt32 getId()          const { return _id; }
    size_t numArgs()        const { return _numArgs; }
    bool   isInitialized()  const { return _initialized; }

    void init(UInt32 id, size_t nArgs)
    {
        _id = id;
        _numArgs = nArgs;
        _initialized = true;
    }

private:
    UInt32 _id;
    size_t _numArgs;
    bool   _initialized;
};

// SqlStatement
class SqlStatement
{
public:
    virtual ~SqlStatement() = default;

    UInt32 getId()    const { return _stmtId.getId(); }
    size_t numArgs()  const { return _stmtId.numArgs(); }

    virtual bool execute() = 0;
    virtual bool directExecute() = 0;

    void addBool(bool v) { arg(v); }
    void addUInt8(UInt8 v) { arg(v); }
    void addInt8(Int8 v) { arg(v); }
    void addUInt16(UInt16 v) { arg(v); }
    void addInt16(Int16 v) { arg(v); }
    void addUInt32(UInt32 v) { arg(v); }
    void addInt32(Int32 v) { arg(v); }
    void addUInt64(UInt64 v) { arg(v); }
    void addInt64(Int64 v) { arg(v); }
    void addFloat(float v) { arg(v); }
    void addDouble(double v) { arg(v); }
    void addString(const char* v) { arg(v); }
    void addString(std::string v) { arg(std::move(v)); }
    void addString(std::ostringstream& ss) { arg(ss.str()); ss.str({}); }
    void addBinary(const UInt8* d, size_t s) { arg(d, s); }
    void addBinary(ByteVector d) { arg(std::move(d)); }

protected:
    SqlStatementID    _stmtId;
    SqlStmtParameters _params;

private:
    SqlStmtParameters& get() { _params.reserve(numArgs()); return _params; }

    template<typename T>
    void arg(T val) { get().addParam(SqlStmtField(val)); }

    template<typename T1, typename T2>
    void arg(T1 v1, T2 v2) { get().addParam(SqlStmtField(v1, v2)); }
};
