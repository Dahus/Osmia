/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#include "DirectOsmiaApp.h"
#include "Shared/Library/Database/DatabaseLoader.h"
#include "Shared/Server/Log/Logger.h"

DirectOsmiaApp::DirectOsmiaApp(string suffixDir) : OsmiaApp(suffixDir) {}

bool DirectOsmiaApp::initialiseService()
{
    Log::debug("DirectOsmiaApp: initialiseService start");

    try
    {
        Log::debug("DirectOsmiaApp: appDir={}", getAppDir());

        _charDb = DatabaseLoader::create(config());

        Log::debug("DirectOsmiaApp: database created");

        if (!_charDb->initialise(DatabaseLoader::makeConnParams(config())))
        {
            Log::error("DirectOsmiaApp: database initialise failed");
            return false;
        }

        Log::info("DirectOsmiaApp: database initialised");
    }
    catch (const std::exception& e)
    {
        Log::error("DirectOsmiaApp: database exception: {}", e.what());
        return false;
    }


    if (!initialiseLua(_charDb.get(), getAppDir() + "/osmia.lua"))
    {
        Log::error("DirectOsmiaApp: failed to load osmia.lua");
        return false;
    }
    return true;
}
