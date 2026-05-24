/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#include "Timer.h"

namespace
{
    const auto _startTime = std::chrono::steady_clock::now();
}

UInt64 GlobalTimer::getMSTime64()
{
    auto now = std::chrono::steady_clock::now();
    return static_cast<UInt64>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now - _startTime).count());
}

UInt32 GlobalTimer::getMSTime()
{
    return static_cast<UInt32>(getMSTime64() % UInt64(0x00000000FFFFFFFF));
}

Int32 GlobalTimer::getTime()
{
    return static_cast<Int32>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
}
