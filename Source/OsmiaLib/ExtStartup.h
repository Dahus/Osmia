/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "OsmiaApp.h"
#include <functional>

namespace ExtStartup
{
    typedef std::function<OsmiaApp* (string profileFolder)> MakeAppFunction;
    void InitModule(MakeAppFunction makeAppFunc);
    void ProcessShutdown();
    void SetupDllSearchPath();
};

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#   include <shellapi.h>
#else
#   define CALLBACK
#endif

extern "C"
{
#ifdef _WIN32
    __declspec(dllexport)
#else
    __attribute__((visibility("default")))
#endif
        void CALLBACK RVExtension(char* output, int outputSize, const char* function);
};

