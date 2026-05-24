/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "Shared/Common/Types.h"
#include <chrono>

class GlobalTimer
{
public:
    static UInt64 getMSTime64();
    static UInt32 getMSTime();
    static UInt32 getMSTimeDiff(UInt32 oldMSTime, UInt32 newMSTime)
    {
        if (oldMSTime > newMSTime)
        {
            const UInt32 diff_1 = (UInt32(0xFFFFFFFF) - oldMSTime) + newMSTime;
            const UInt32 diff_2 = oldMSTime - newMSTime;
            return std::min(diff_1, diff_2);
        }
        return newMSTime - oldMSTime;
    }
    static Int32 getTime();

private:
    GlobalTimer() = delete;
};
