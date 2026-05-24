/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "Shared/Common/Types.h"
#include "OsmiaLib/OsmiaApp.h"

class Database;

class DirectOsmiaApp : public OsmiaApp
{
public:
    DirectOsmiaApp(string suffixDir);
protected:
    bool initialiseService() override;
private:
    shared_ptr<Database> _charDb;
};
