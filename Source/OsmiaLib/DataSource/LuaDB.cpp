/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#include "LuaDB.h"
#include "AsyncQueryPool.h"
#include "Database/Database.h"
#include "Shared/Server/Log/Logger.h"

LuaDB::LuaDB(lua_State* L, Database* db)
    : _lua(L), _db(db), _pool(std::make_unique<AsyncQueryPool>(db))
{
    registerTable();
    Log::info("LuaDB: initialized");
}

void LuaDB::registerTable()
{
    // Store this in registry so static methods can reach it
    lua_pushlightuserdata(_lua, this);
    lua_setfield(_lua, LUA_REGISTRYINDEX, "LuaDB");

    // Build db table
    lua_newtable(_lua);

    lua_pushcfunction(_lua, lua_query);
    lua_setfield(_lua, -2, "query");

    lua_pushcfunction(_lua, lua_execute);
    lua_setfield(_lua, -2, "execute");

    lua_pushcfunction(_lua, lua_queryAsync);
    lua_setfield(_lua, -2, "queryAsync");

    lua_pushcfunction(_lua, lua_queryStatus);
    lua_setfield(_lua, -2, "queryStatus");

    lua_pushcfunction(_lua, lua_queryFetch);
    lua_setfield(_lua, -2, "queryFetch");

    lua_pushcfunction(_lua, lua_queryClose);
    lua_setfield(_lua, -2, "queryClose");

    lua_pushcfunction(_lua, lua_transaction);
    lua_setfield(_lua, -2, "transaction");

    lua_pushcfunction(_lua, lua_executeBatch);
    lua_setfield(_lua, -2, "executeBatch");

    lua_setglobal(_lua, "db");
}

// Helper: get LuaDB* from registry
static LuaDB* getDB(lua_State* L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, "LuaDB");
    LuaDB* db = static_cast<LuaDB*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return db;
}

// db.query(sql) -> table of {field=value, ...} rows, or nil
int LuaDB::lua_query(lua_State* L)
{
    LuaDB* self = getDB(L);
    const char* sql = luaL_checkstring(L, 1);

    auto res = self->_db->query(sql);
    if (!res)
    {
        lua_pushnil(L);
        return 1;
    }

    // Fetch field names BEFORE iterating rows
    QueryFieldNames fields = res->fetchFieldNames();
    size_t numFields = res->numFields();

    lua_newtable(L);
    int rowIdx = 1;
    while (res->fetchRow())
    {
        lua_newtable(L);
        for (size_t i = 0; i < numFields; ++i)
        {
            lua_pushstring(L, res->at(i).getString().c_str());
            if (i < fields.size())
                lua_setfield(L, -2, fields[i].c_str());
            else
                lua_rawseti(L, -2, static_cast<int>(i) + 1);
        }
        lua_rawseti(L, -2, rowIdx++);
    }
    return 1;
}

// db.execute(sql) -> bool
int LuaDB::lua_execute(lua_State* L)
{
    LuaDB* self = getDB(L);
    const char* sql = luaL_checkstring(L, 1);
    bool ok = self->_db->execute(sql);
    lua_pushboolean(L, ok ? 1 : 0);
    return 1;
}

// db.queryAsync(sql) -> token
int LuaDB::lua_queryAsync(lua_State* L)
{
    LuaDB* self = getDB(L);
    const char* sql = luaL_checkstring(L, 1);
    UInt32 token = self->_pool->submit(sql);
    lua_pushinteger(L, static_cast<lua_Integer>(token));
    return 1;
}

// db.queryStatus(token) -> 0=pending, 1=ready, -1=error
int LuaDB::lua_queryStatus(lua_State* L)
{
    LuaDB* self = getDB(L);
    UInt32 token = static_cast<UInt32>(luaL_checkinteger(L, 1));
    int status = self->_pool->status(token);
    lua_pushinteger(L, status);
    return 1;
}

// db.queryFetch(token) -> {field=value,...} or nil (no more rows)
int LuaDB::lua_queryFetch(lua_State* L)
{
    LuaDB* self = getDB(L);
    UInt32 token = static_cast<UInt32>(luaL_checkinteger(L, 1));

    auto names = self->_pool->fieldNames(token);
    auto row = self->_pool->fetchRow(token);

    if (!row.has_value())
    {
        // Token unknown or not ready yet
        lua_pushnil(L);
        return 1;
    }

    if (row->empty())
    {
        // No more rows — return nil
        lua_pushnil(L);
        return 1;
    }

    lua_newtable(L);
    for (size_t i = 0; i < row->size(); ++i)
    {
        lua_pushstring(L, (*row)[i].c_str());
        // Use field name if available, else numeric key
        if (names.has_value() && i < names->size())
            lua_setfield(L, -2, (*names)[i].c_str());
        else
            lua_rawseti(L, -2, static_cast<int>(i) + 1);
    }
    return 1;
}

// db.queryClose(token) -> bool
int LuaDB::lua_queryClose(lua_State* L)
{
    LuaDB* self = getDB(L);
    UInt32 token = static_cast<UInt32>(luaL_checkinteger(L, 1));
    bool ok = self->_pool->close(token);
    lua_pushboolean(L, ok ? 1 : 0);
    return 1;
}

// db.transaction(fn) -> true, or raises Lua error on failure
int LuaDB::lua_transaction(lua_State* L)
{
    LuaDB* self = getDB(L);
    luaL_checktype(L, 1, LUA_TFUNCTION);

    // BEGIN
    if (!self->_db->execute("BEGIN"))
        return luaL_error(L, "db.transaction: BEGIN failed");

    // Call fn() via pcall so we catch Lua errors
    lua_pushvalue(L, 1);  // copy fn to top
    int pcallResult = lua_pcall(L, 0, 0, 0);

    if (pcallResult != LUA_OK)
    {
        // Grab error message before ROLLBACK touches the stack
        std::string err = lua_tostring(L, -1);
        lua_pop(L, 1);

        self->_db->execute("ROLLBACK");
        Log::error("LuaDB: transaction rolled back: {}", err);

        // Re-raise so callHandler sees it
        return luaL_error(L, "db.transaction rolled back: %s", err.c_str());
    }

    // COMMIT
    if (!self->_db->execute("COMMIT"))
    {
        self->_db->execute("ROLLBACK");
        return luaL_error(L, "db.transaction: COMMIT failed, rolled back");
    }

    lua_pushboolean(L, 1);
    return 1;
}

// db.executeBatch(sqls) -> bool
int LuaDB::lua_executeBatch(lua_State* L)
{
    LuaDB* self = getDB(L);
    luaL_checktype(L, 1, LUA_TTABLE);

    if (!self->_db->execute("BEGIN"))
        return luaL_error(L, "db.executeBatch: BEGIN failed");

    int n = static_cast<int>(lua_rawlen(L, 1));
    for (int i = 1; i <= n; ++i)
    {
        lua_rawgeti(L, 1, i);
        const char* sql = lua_tostring(L, -1);
        lua_pop(L, 1);

        if (!sql)
        {
            self->_db->execute("ROLLBACK");
            return luaL_error(L, "db.executeBatch: nil sql at index %d", i);
        }

        if (!self->_db->execute(sql))
        {
            self->_db->execute("ROLLBACK");
            Log::error("LuaDB: executeBatch failed at index {}: {}", i, sql);
            return luaL_error(L, "db.executeBatch: execute failed at index %d", i);
        }
    }

    if (!self->_db->execute("COMMIT"))
    {
        self->_db->execute("ROLLBACK");
        return luaL_error(L, "db.executeBatch: COMMIT failed, rolled back");
    }

    Log::trace("LuaDB: executeBatch committed {} statements", n);
    lua_pushboolean(L, 1);
    return 1;
}
