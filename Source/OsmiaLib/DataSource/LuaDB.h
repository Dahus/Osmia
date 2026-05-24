/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <lua.hpp>
#include <memory>

class Database;
class AsyncQueryPool;

class LuaDB
{
public:
    // Registers db.* table into existing Lua state
    // Does NOT own lua_State — LuaEngine owns it
    LuaDB(lua_State* L, Database* db);
    ~LuaDB() = default;

private:
    lua_State* _lua;
    Database* _db;
    std::unique_ptr<AsyncQueryPool> _pool;

    void registerTable();

    // db.query(sql)          -> table of rows (sync)
    static int lua_query(lua_State* L);

    // db.execute(sql)        -> bool
    static int lua_execute(lua_State* L);

    // db.queryAsync(sql)     -> token (uint32)
    static int lua_queryAsync(lua_State* L);

    // db.queryStatus(token)  -> 0=pending, 1=ready, -1=error
    static int lua_queryStatus(lua_State* L);

    // db.queryFetch(token)   -> table (next row) or nil (no more rows)
    static int lua_queryFetch(lua_State* L);

    // db.queryClose(token)   -> bool
    static int lua_queryClose(lua_State* L);

    // db.transaction(fn)     -> bool, calls fn() inside BEGIN/COMMIT
    static int lua_transaction(lua_State* L);

    // db.executeBatch(sqls)  -> bool, executes table of SQL strings in one transaction
    static int lua_executeBatch(lua_State* L);
};
