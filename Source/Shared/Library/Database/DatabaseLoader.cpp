/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#include "DatabaseLoader.h"
#include "Shared/Library/SharedLibraryLoader.h"
#include "Shared/Server/Log/Logger.h"
#include <algorithm>
#include <cctype>

extern "C" Database* createInstance();
extern "C" void destroyInstance(Database* p);

namespace
{
    using LibraryType = SharedLibraryLoader<Database>;

    LibraryType& getHolder()
    {
        static LibraryType instance;
        return instance;
    }

    string toLower(string s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return s;
    }

    string trim(string s)
    {
        auto isSpace = [](unsigned char c) { return std::isspace(c); };
        s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), isSpace));
        s.erase(std::find_if_not(s.rbegin(), s.rend(), isSpace).base(), s.end());
        return s;
    }
}

string DatabaseLoader::getDbType(const IniConfig& config)
{
    string dbType;
    if (config.hasKey("Database", "Type"))          dbType = config.getString("Database", "Type");
    else if (config.hasKey("Database", "Provider")) dbType = config.getString("Database", "Provider");
    else if (config.hasKey("Database", "Engine"))   dbType = config.getString("Database", "Engine");
    else                                         dbType = "ODBC";

    dbType = trim(dbType);
    if (dbType.empty())
        throw CreationError("Unspecified database type");

    return dbType;
}

string DatabaseLoader::getModuleName(const string& dbType)
{
    return "DatabaseODBC";
}

shared_ptr<Database> DatabaseLoader::loadAndCreate(const string& dbType)
{
    const string modName = getModuleName(dbType);
    Log::info("DatabaseLoader: loading module {}", modName);

    Database* raw = createInstance();
    if (!raw)
        throw CreationError("createInstance returned null");
    return shared_ptr<Database>(raw, [](Database* p) { destroyInstance(p); });
}

shared_ptr<Database> DatabaseLoader::create(const IniConfig& config)
{
    return loadAndCreate(getDbType(config));
}

Database::KeyValueColl DatabaseLoader::makeConnParams(const IniConfig& config)
{
    Database::KeyValueColl keyVals;

    static const char* keys[] = {
        "Host", "Port", "Database", "Username", "Password", "Type", "ConnectionString"
    };

    for (auto& key : keys)
    {
        if (config.hasKey("Database", key))
            keyVals[toLower(key)] = trim(config.getString("Database", key));
    }

    return keyVals;
}
