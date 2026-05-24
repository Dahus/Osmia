/*
 * Copyright (C) 2026 Kronus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <lua.hpp>
#include <string>
#include "LuaDB.h"

class Database;

class LuaEngine
{
public:
    LuaEngine(Database* db);
    ~LuaEngine();

    bool        loadFile(const std::string& path);
    std::string callHandler(int handlerNum, const std::string& params);
    bool        hasHandler(int handlerNum) const;

private:
    lua_State* _lua;
    Database* _db;
    std::unique_ptr<LuaDB>  _luaDB;

    void registerApi();

    static int lua_log(lua_State* L);
};
