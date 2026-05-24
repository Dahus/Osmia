/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "Shared/Common/Types.h"
#include "Shared/Server/Config/IniConfig.h"
#include "Shared/Server/Log/Logger.h"
#include <string>

class AppServer
{
public:
    AppServer(std::string appName = "", std::string appDir = "");
    virtual ~AppServer();

    const std::string& getAppDir()  const { return _appDir; }
    const std::string& getAppName() const { return _appName; }
    const IniConfig& config()     const { return _config; }

    // Call once at startup
    bool initialize();

    // Call at shutdown
    void uninitialize();

    AppServer(const AppServer&) = delete;
    AppServer& operator=(const AppServer&) = delete;

protected:
    virtual void onInitialize() {}
    virtual void onUninitialize() {}

private:
    void initConfig();
    void initLogger();

    std::string _appName;
    std::string _appDir;
    IniConfig   _config;
};
