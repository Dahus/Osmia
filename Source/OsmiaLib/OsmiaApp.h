/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include "Shared/Common/Types.h"
#include "Shared/Server/AppServer.h"
#include "DataSource/LuaEngine.h"
#include "Database/Database.h"
#include <memory>
#include <map>
#include <vector>
#include <string>

class Database;

struct QueryData
{
    std::vector<std::string> fieldNames;
    std::vector<std::vector<std::string>> rows;
    size_t currentRow = 0;
};

class OsmiaApp : public AppServer
{
public:
    OsmiaApp(string suffixDir);
    virtual ~OsmiaApp() {};
    void callExtension(const char* function, char* output, size_t outputSize);
    bool isShutdownRequested() const { return _shutdownRequested; }

protected:
    void onInitialize() override
    {
        try
        {
            if (!initialiseService())
                Log::fatal("OsmiaApp: initialiseService returned false");
        }
        catch (const std::exception& e)
        {
            Log::fatal("OsmiaApp: onInitialize exception: {}", e.what());
        }
        catch (...)
        {
            Log::fatal("OsmiaApp: onInitialize unknown exception");
        }
    }
    virtual bool initialiseService() = 0;
    bool initialiseLua(Database* db, const std::string& scriptPath);

protected:
    unique_ptr<LuaEngine> _lua;
    Database* _db = nullptr;
    bool _shutdownRequested = false;

    // Query state
    uint32_t _tokenCounter = 0;
    map<uint32_t, QueryData> _queries;
};
