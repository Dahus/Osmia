/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "IConsoleChannel.h"
#ifdef _WIN32
#   include <windows.h>
#   include <atomic>
#endif
class ArmaConsoleChannel : public IConsoleChannel
{
public:
    ArmaConsoleChannel();
    void log(Log::Level level, std::string_view msg) override;
private:
#ifdef _WIN32
    std::atomic<HWND> _wndRich;
    static HWND findRichEdit();
#endif
};