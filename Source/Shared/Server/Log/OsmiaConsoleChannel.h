/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "IConsoleChannel.h"
#include <string>
#ifdef _WIN32
#   include <windows.h>
#endif

class OsmiaConsoleChannel : public IConsoleChannel
{
public:
    explicit OsmiaConsoleChannel(std::string windowTitle = "");
    ~OsmiaConsoleChannel() override;
    void log(Log::Level level, std::string_view msg) override;
    OsmiaConsoleChannel(const OsmiaConsoleChannel&) = delete;
    OsmiaConsoleChannel& operator=(const OsmiaConsoleChannel&) = delete;
private:
    bool _consoleCreated;
#ifdef _WIN32
    HANDLE _hConsole;
#endif
};
