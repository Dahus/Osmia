/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#include "IniConfig.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace
{
    std::string trim(std::string_view sv)
    {
        size_t start = 0;
        size_t end = sv.size();
        while (start < end && std::isspace(static_cast<unsigned char>(sv[start]))) ++start;
        while (end > start && std::isspace(static_cast<unsigned char>(sv[end - 1]))) --end;
        return std::string(sv.substr(start, end - start));
    }

    std::string toLower(std::string_view sv)
    {
        std::string result(sv);
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return result;
    }
}

bool IniConfig::load(std::string_view path)
{
    std::ifstream file(path.data());
    if (!file.is_open()) return false;

    std::string currentSection;
    std::string line;

    while (std::getline(file, line))
    {
        // Strip comments
        if (auto pos = line.find(';'); pos != std::string::npos)
            line = line.substr(0, pos);
        if (auto pos = line.find('#'); pos != std::string::npos)
            line = line.substr(0, pos);

        std::string trimmed = trim(line);
        if (trimmed.empty()) continue;

        // Section
        if (trimmed.front() == '[' && trimmed.back() == ']')
        {
            currentSection = trim(trimmed.substr(1, trimmed.size() - 2));
            continue;
        }

        // Key = Value
        auto eq = trimmed.find('=');
        if (eq == std::string::npos) continue;

        std::string key = trim(trimmed.substr(0, eq));
        std::string value = trim(trimmed.substr(eq + 1));

        if (!key.empty())
            _data[currentSection][key] = value;
    }

    return true;
}

std::optional<std::string> IniConfig::get(std::string_view section, std::string_view key) const
{
    auto sit = _data.find(std::string(section));
    if (sit == _data.end()) return std::nullopt;

    auto kit = sit->second.find(std::string(key));
    if (kit == sit->second.end()) return std::nullopt;

    return kit->second;
}

std::string IniConfig::getString(std::string_view section, std::string_view key,
    std::string_view defaultVal) const
{
    return get(section, key).value_or(std::string(defaultVal));
}

bool IniConfig::getBool(std::string_view section, std::string_view key, bool defaultVal) const
{
    auto val = get(section, key);
    if (!val) return defaultVal;
    auto lower = toLower(*val);
    return lower == "true" || lower == "1" || lower == "yes";
}

int IniConfig::getInt(std::string_view section, std::string_view key, int defaultVal) const
{
    auto val = get(section, key);
    if (!val) return defaultVal;
    try { return std::stoi(*val); }
    catch (...) { return defaultVal; }
}

bool IniConfig::hasKey(std::string_view section, std::string_view key) const
{
    return get(section, key).has_value();
}
