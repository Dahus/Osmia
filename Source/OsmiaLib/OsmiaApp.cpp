 /*
  * Copyright (C) 2026 Kronus
  * SPDX-License-Identifier: MIT
  */
#include "OsmiaApp.h"
#include "Database/Database.h"
#include "Shared/Server/Log/Logger.h"
#include <cstring>

OsmiaApp::OsmiaApp(string suffixDir) : AppServer("Osmia", suffixDir) {}

bool OsmiaApp::initialiseLua(Database* db, const std::string& scriptPath)
{
    _db = db;
    _lua = std::make_unique<LuaEngine>(db);
    Log::debug("LuaEngine: created");
    if (!_lua->loadFile(scriptPath))
    {
        Log::error("LuaEngine: failed to load '{}'", scriptPath);
        return false;
    }
    Log::info("LuaEngine: loaded '{}'", scriptPath);
    return true;
}

void OsmiaApp::callExtension(const char* function, char* output, size_t outputSize)
{
    Log::trace("callExtension: {}", function);

    string input(function);
    size_t first = input.find(':');
    if (first == string::npos || input.substr(0, first) != "CHILD")
    {
        Log::error("Invalid format: {}", input);
        return;
    }
    size_t second = input.find(':', first + 1);
    if (second == string::npos)
    {
        Log::error("No handler number: {}", input);
        return;
    }
    int handlerNum = -1;
    try { handlerNum = std::stoi(input.substr(first + 1, second - first - 1)); }
    catch (...) { Log::error("Bad handler number: {}", input); return; }

    string handlerParams = second + 1 < input.size() ? input.substr(second + 1) : "";

    Log::trace("Handler: {} params: {}", handlerNum, handlerParams);

    string result;
    try
    {
        if (!_lua->hasHandler(handlerNum))
        {
            Log::error("No handler: {}", handlerNum);
            return;
        }

        result = _lua->callHandler(handlerNum, handlerParams);
        Log::trace("Handler {} result: {}", handlerNum, result);

        if (result == "__SHUTDOWN__")
        {
            Log::info("Osmia: shutdown requested by handler {}", handlerNum);
            _shutdownRequested = true;
            result = "[\"PASS\"]";
        }
    }
    catch (const std::exception& e) { Log::error("Handler {} exception: {}", handlerNum, e.what()); return; }
    catch (...) { Log::error("Handler {} unknown exception", handlerNum); return; }

    if (result.length() >= outputSize)
        Log::error("Output too big for handler: {}", handlerNum);
    else
#ifdef _WIN32
        strncpy_s(output, outputSize, result.c_str(), outputSize - 1);
#else
        strncpy(output, result.c_str(), outputSize - 1);
    output[outputSize - 1] = '\0';
#endif
}
