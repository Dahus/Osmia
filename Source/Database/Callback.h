/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "QueryResult.h"
#include <functional>
#include <memory>

struct QueryCallback
{
    using ResType = std::unique_ptr<QueryResult>;
    using FuncType = std::function<void(ResType)>;

    QueryCallback() = default;

    explicit QueryCallback(FuncType fun, QueryResult* res = nullptr)
        : _fun(std::move(fun))
        , _res(res)
    {}

    void invoke()
    {
        ResType pRes(std::move(_res));
        if (_fun)
            _fun(std::move(pRes));
    }

    void setResult(QueryResult* res) { _res.reset(res); }

    bool valid() const { return _fun != nullptr; }

private:
    FuncType _fun;
    ResType  _res;
};
