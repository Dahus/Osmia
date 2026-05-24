/*
 * Copyright (C) 2026 Kronus
 * SPDX-License-Identifier: MIT
 */
#include "LuaEngine.h"
#include "AsyncQueryPool.h"
#include "Shared/Server/Log/Logger.h"
#include <filesystem>

namespace fs = std::filesystem;

LuaEngine::LuaEngine(Database* db)
    : _db(db), _lua(nullptr)
{
    _lua = luaL_newstate();
    if (!_lua)
    {
        Log::error("LuaEngine: failed to create Lua state");
        return;
    }

    luaL_openlibs(_lua);
    registerApi();

    lua_newtable(_lua);
    lua_setglobal(_lua, "handlers");

    Log::info("LuaEngine: initialized");
}

LuaEngine::~LuaEngine()
{
    if (_lua)
    {
        lua_close(_lua);
        _lua = nullptr;
    }
}

bool LuaEngine::loadFile(const std::string& path)
{
    if (!_lua) return false;

    fs::path p(path);
    std::string folder = p.parent_path().string();
    if (!folder.empty() && folder.back() != fs::path::preferred_separator)
        folder += fs::path::preferred_separator;

    lua_pushstring(_lua, folder.c_str());
    lua_setglobal(_lua, "serverFolder");

    int res = luaL_dofile(_lua, path.c_str());
    if (res != LUA_OK)
    {
        std::string err = lua_tostring(_lua, -1);
        lua_pop(_lua, 1);
        Log::error("LuaEngine: error loading '{}': {}", path, err);
        return false;
    }

    Log::info("LuaEngine: loaded '{}'", path);
    return true;
}

bool LuaEngine::hasHandler(int handlerNum) const
{
    if (!_lua) return false;

    lua_getglobal(_lua, "handlers");
    lua_pushinteger(_lua, handlerNum);
    lua_gettable(_lua, -2);

    bool exists = lua_isfunction(_lua, -1);
    lua_pop(_lua, 2);
    return exists;
}

std::string LuaEngine::callHandler(int handlerNum, const std::string& params)
{
    if (!_lua) return "[\"ERROR\"]";

    lua_getglobal(_lua, "handlers");
    lua_pushinteger(_lua, handlerNum);
    lua_gettable(_lua, -2);
    lua_remove(_lua, -2);

    if (!lua_isfunction(_lua, -1))
    {
        lua_pop(_lua, 1);
        Log::error("LuaEngine: no handler for {}", handlerNum);
        return "[\"ERROR\"]";
    }

    lua_pushstring(_lua, params.c_str());

    if (lua_pcall(_lua, 1, 1, 0) != LUA_OK)
    {
        std::string err = lua_tostring(_lua, -1);
        lua_pop(_lua, 1);
        Log::error("LuaEngine: handler {} error: {}", handlerNum, err);
        return "[\"ERROR\"]";
    }

    std::string result = "[\"ERROR\"]";
    if (lua_isstring(_lua, -1))
        result = lua_tostring(_lua, -1);

    lua_pop(_lua, 1);
    return result;
}

void LuaEngine::registerApi()
{
    lua_pushlightuserdata(_lua, this);
    lua_setfield(_lua, LUA_REGISTRYINDEX, "LuaEngine");

    _luaDB = std::make_unique<LuaDB>(_lua, _db);

    lua_newtable(_lua);
    lua_pushcfunction(_lua, lua_log);
    lua_setfield(_lua, -2, "info");
    lua_setglobal(_lua, "log");
}

int LuaEngine::lua_log(lua_State* L)
{
    const char* msg = luaL_checkstring(L, 1);
    Log::info("Lua: {}", msg);
    return 0;
}
