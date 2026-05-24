/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#include "AppServer.h"
#include "Shared/Server/Log/ArmaConsoleChannel.h"
#include "Shared/Server/Log/OsmiaConsoleChannel.h"
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

AppServer::AppServer(std::string appName, std::string appDir)
    : _appName(std::move(appName))
    , _appDir(std::move(appDir))
{}

AppServer::~AppServer()
{
    uninitialize();
}

bool AppServer::initialize()
{
    if (!_appDir.empty())
        _appDir = fs::absolute(_appDir).string();
    else
        _appDir = fs::current_path().string();

    if (_appDir.back() != fs::path::preferred_separator)
        _appDir += fs::path::preferred_separator;

    initConfig();
    initLogger();
    Log::debug("AppServer: appDir={}", _appDir);
    onInitialize();
    return true;
}

void AppServer::uninitialize()
{
    onUninitialize();
    Log::shutdown();
}

void AppServer::initConfig()
{
    std::string path = _appDir + "osmia.ini";
    if (!_config.load(path))
        std::cerr << std::format("Unable to load configuration: {}\n", path);
}

void AppServer::initLogger()
{
    std::string logName = _appName;
    std::transform(logName.begin(), logName.end(), logName.begin(), ::tolower);
    std::string logFile = _appDir + logName + ".log";
    bool        async = _config.getBool("Logger", "Async", false);
    Log::Level  level = Log::parseLevel(_config.getString("Logger", "Level", "info"));

    Log::Level  conLevel = level;
    if (_config.hasKey("Logger", "ConsoleLevel"))
        conLevel = Log::parseLevel(_config.getString("Logger", "ConsoleLevel", "info"));

    Log::init(logFile, level, conLevel, async);

    // Console channel
    bool useRealConsole = true;
#ifndef _DEBUG
    useRealConsole = _config.getBool("Logger", "SeparateConsole", false);
#endif

    if (useRealConsole)
    {
        std::string title = _appName;
        fs::path dirPath(_appDir);
        if (dirPath.has_parent_path())
            title += " - " + dirPath.parent_path().filename().string();

        Log::setConsoleChannel(std::make_unique<OsmiaConsoleChannel>(std::move(title)));
    }
    else
    {
        Log::setConsoleChannel(std::make_unique<ArmaConsoleChannel>());
    }

    Log::info("Logger initialised — level: {}, async: {}",
        _config.getString("Logger", "Level", "info"),
        async ? "true" : "false");
}
