/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <optional>

class IniConfig
{
public:
    // Load from file, returns false if file not found
    bool load(std::string_view path);

    // Get value by "Section.Key"
    std::optional<std::string> get(std::string_view section, std::string_view key) const;

    // Get with default
    std::string getString(std::string_view section, std::string_view key,
        std::string_view defaultVal = "") const;

    bool        getBool(std::string_view section, std::string_view key, bool        defaultVal = false) const;
    int         getInt(std::string_view section, std::string_view key, int         defaultVal = 0)     const;

    bool hasKey(std::string_view section, std::string_view key) const;

private:
    // "Section" -> { "Key" -> "Value" }
    std::unordered_map<std::string,
        std::unordered_map<std::string, std::string>> _data;
};
