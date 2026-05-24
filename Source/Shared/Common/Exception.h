/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <stdexcept>
#include <string>
#include <ostream>
#include <Poco/Logger.h>
#include <Poco/Exception.h>

template <typename ExceptType>
class GenericException : public ExceptType
{
public:
    explicit GenericException(const char* staticName) : ExceptType(staticName) {}
    virtual ~GenericException() noexcept = default;
    virtual std::string toString() const = 0;
    const char* what() const noexcept override { _str = this->toString(); return _str.c_str(); }
    void print(Poco::Logger& logger) const { logger.error(this->toString()); }
    void print(std::ostream& out) const { out << this->toString() << std::endl; }
private:
    mutable std::string _str;
};

#define POCO_DEFINE_EXCEPTION_CODE(API, CLS, BASE, CODE, NAME)              \
class API CLS : public BASE                                                 \
{                                                                           \
public:                                                                     \
    CLS(int code = CODE) : BASE(code) {}                                    \
    CLS(const std::string& msg, int code = CODE) : BASE(msg, code) {}       \
    CLS(const std::string& msg, const std::string& arg, int code = CODE)    \
        : BASE(msg, arg, code) {}                                           \
    CLS(const std::string& msg, const Poco::Exception& exc, int code = CODE)\
        : BASE(msg, exc, code) {}                                           \
    CLS(const CLS& exc) : BASE(exc) {}                                      \
    ~CLS() noexcept override = default;                                     \
    CLS& operator=(const CLS& exc) { BASE::operator=(exc); return *this; }  \
    const char* name() const noexcept override { return NAME; }             \
    const char* className() const noexcept override                         \
        { return typeid(*this).name(); }                                    \
    Poco::Exception* clone() const override { return new CLS(*this); }      \
    void rethrow() const override { throw *this; }                          \
};

#define POCO_DEFINE_EXCEPTION(API, CLS, BASE, NAME)                         \
    POCO_DEFINE_EXCEPTION_CODE(API, CLS, BASE, 0, NAME)CODE)                \
        : BASE(msg, arg, code) {}                                           \
    CLS(const std::string& msg, const Poco::Exception& exc, int code = CODE)\
        : BASE(msg, exc, code) {}                                           \
    CLS(const CLS& exc) : BASE(exc) {}                                      \
    ~CLS() noexcept override = default;                                     \
    CLS& operator=(const CLS& exc) { BASE::operator=(exc); return *this; }  \
    const char* name() const noexcept override { return NAME; }             \
    const char* className() const noexcept override                         \
        { return typeid(*this).name(); }                                    \
    Poco::Exception* clone() const override { return new CLS(*this); }      \
    void rethrow() const override { throw *this; }                          \
};

#define POCO_DEFINE_EXCEPTION(API, CLS, BASE, NAME) \
    POCO_DEFINE_EXCEPTION_CODE(API, CLS, BASE, 0, NAME)
