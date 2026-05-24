/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "Shared/Common/Types.h"
#include "Database/Database.h"
#include "Shared/Server/Config/IniConfig.h"
#include <stdexcept>

class DatabaseLoader
{
public:
    struct CreationError : std::runtime_error
    {
        explicit CreationError(const string& msg)
            : std::runtime_error("Cannot create database: " + msg) {}
    };

    static shared_ptr<Database> create(const IniConfig& config);
    static Database::KeyValueColl makeConnParams(const IniConfig& config);

private:
    static string getDbType(const IniConfig& config);
    static string getModuleName(const string& dbType);
    static shared_ptr<Database> loadAndCreate(const string& dbType);
};
