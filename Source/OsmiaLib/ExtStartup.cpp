/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */

#include "Shared/Server/Log/Logger.h"
#ifndef _WIN32
#   define CALLBACK
#endif
#include "ExtStartup.h"
#include <filesystem>

namespace fs = std::filesystem;

namespace
{
#ifdef _WIN32
    string getServerFolder()
    {
        wchar_t buf[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, buf, MAX_PATH);
        fs::path exePath(buf);
        return (exePath.parent_path() / "Osmia").string();
    }
#else
    static string getServerFolder()
    {
        char buf[4096] = {};
        ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (len <= 0) return "osmia";
        fs::path exePath(buf);
        return (exePath.parent_path() / "Osmia").string();
    }
#endif

    ExtStartup::MakeAppFunction gMakeAppFunc;

    unique_ptr<OsmiaApp> CreateApp()
    {
        string serverFolder = getServerFolder();
        Log::info("CreateApp: serverFolder={}", serverFolder);
        unique_ptr<OsmiaApp> theApp(gMakeAppFunc(serverFolder));
        if (!theApp->initialize())
        {
            Log::fatal("Osmia startup failed — initialize() returned false");
            return nullptr;
        }
        return theApp;
    }

    unique_ptr<OsmiaApp> gApp;
}

void ExtStartup::SetupDllSearchPath()
{
#ifdef _WIN32
    string folder = getServerFolder();
    std::wstring wfolder(folder.begin(), folder.end());
    SetDllDirectoryW(wfolder.c_str());
    LoadLibraryW((wfolder + L"\\lua.dll").c_str());
#endif
}

void ExtStartup::InitModule(MakeAppFunction makeAppFunc)
{
    gMakeAppFunc = std::move(makeAppFunc);
}

void ExtStartup::ProcessShutdown()
{
    gApp.reset();
}

void CALLBACK RVExtension(char* output, int outputSize, const char* function)
{
    if (!gApp)
    {
        Log::info("ExtStartup: first call, creating app");
        gApp = CreateApp();
        if (!gApp)
#ifdef _WIN32
            ExitProcess(1);
#else
            exit(1);
#endif
    }
    Log::trace("RVExtension: {}", function);
    gApp->callExtension(function, output, outputSize);
    if (gApp->isShutdownRequested())
        ExtStartup::ProcessShutdown();
}