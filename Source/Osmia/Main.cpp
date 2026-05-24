/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */

#include "OsmiaLib/ExtStartup.h"
#include "DirectOsmiaApp.h"
#include <fstream>

#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        ExtStartup::SetupDllSearchPath();
        ExtStartup::InitModule([](string profileFolder) { return new DirectOsmiaApp(profileFolder); });
        break;
    }
    case DLL_PROCESS_DETACH:
        ExtStartup::ProcessShutdown();
        break;
    }
    return TRUE;
}
#else
__attribute__((constructor))
static void soLoad()
{
    ExtStartup::InitModule([](string profileFolder) { return new DirectOsmiaApp(profileFolder); });
}
__attribute__((destructor))
static void soUnload()
{
    ExtStartup::ProcessShutdown();
}
#endif
