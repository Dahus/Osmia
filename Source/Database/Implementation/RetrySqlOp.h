/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "SqlConnection.h"
#include "Shared/Common/Timer.h"
#include <functional>
#include <format>
#include <string>

namespace Retry
{
    template<typename RetVal>
    struct SqlOp
    {
        using FuncType = std::function<RetVal(SqlConnection&)>;
        using SqlStrFunc = std::function<std::string()>;

        explicit SqlOp(FuncType func, bool throwExc = false)
            : _func(std::move(func))
            , _throwExc(throwExc)
        {}

        RetVal operator()(SqlConnection& conn,
            const char* logStr = "",
            SqlStrFunc  sqlLogFunc = []() { return std::string{}; })
        {
            for (;;)
            {
                try
                {
                    UInt32 startTime = GlobalTimer::getMSTime();
                    RetVal result = _func(conn);

                    if (logStr && logStr[0] != '\0')
                    {
                        UInt32 elapsed = GlobalTimer::getMSTimeDiff(startTime, GlobalTimer::getMSTime());
                        std::string sql = sqlLogFunc();
                        std::string msg = sql.empty()
                            ? std::format("{} [{} ms]", logStr, elapsed)
                            : std::format("{} [{} ms] SQL: '{}'", logStr, elapsed, sql);

                        // caller logs via their own logger
                        (void)msg;
                    }

                    return result;
                }
                catch (const SqlConnection::SqlException& e)
                {
                    if (e.isConnLost())
                    {
                        try { conn.connect(); }
                        catch (const SqlConnection::SqlException&)
                        {
                            if (_throwExc) throw;
                            return RetVal{};
                        }
                    }
                    if (_throwExc) throw;
                    if (e.isRepeatable()) continue;
                    return RetVal{};
                }
            }
        }

    private:
        FuncType _func;
        bool     _throwExc;
    };
}
