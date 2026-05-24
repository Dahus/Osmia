/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "Logger.h"
#include <string_view>

class IConsoleChannel
{
public:
    virtual ~IConsoleChannel() = default;
    virtual void log(Log::Level level, std::string_view msg) = 0;

    IConsoleChannel(const IConsoleChannel&) = delete;
    IConsoleChannel& operator=(const IConsoleChannel&) = delete;

protected:
    IConsoleChannel() = default;
};
