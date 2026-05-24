/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <version>
#include <string>
#include <string_view>
#include <format>
#include <memory>

class IConsoleChannel;

namespace Log
{
    enum class Level
    {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warning = 3,
        Error = 4,
        Fatal = 5,
        None = 6
    };

    void init(std::string_view logFilePath, Level fileLevel = Level::Info,
        Level consoleLevel = Level::Info, bool async = false);
    void shutdown();

    void setConsoleChannel(std::unique_ptr<IConsoleChannel> channel);

    void setFileLevel(Level level);
    void setConsoleLevel(Level level);

    void log(Level level, std::string_view msg);

    inline void trace(std::string_view msg) { log(Level::Trace, msg); }
    inline void debug(std::string_view msg) { log(Level::Debug, msg); }
    inline void info(std::string_view msg) { log(Level::Info, msg); }
    inline void warning(std::string_view msg) { log(Level::Warning, msg); }
    inline void error(std::string_view msg) { log(Level::Error, msg); }
    inline void fatal(std::string_view msg) { log(Level::Fatal, msg); }

    template<typename... Args>
    void trace(std::string_view fmt, Args&&... args)
    {
        log(Level::Trace, std::vformat(fmt, std::make_format_args(args...)));
    }
    template<typename... Args>
    void debug(std::string_view fmt, Args&&... args)
    {
        log(Level::Debug, std::vformat(fmt, std::make_format_args(args...)));
    }
    template<typename... Args>
    void info(std::string_view fmt, Args&&... args)
    {
        log(Level::Info, std::vformat(fmt, std::make_format_args(args...)));
    }
    template<typename... Args>
    void warning(std::string_view fmt, Args&&... args)
    {
        log(Level::Warning, std::vformat(fmt, std::make_format_args(args...)));
    }
    template<typename... Args>
    void error(std::string_view fmt, Args&&... args)
    {
        log(Level::Error, std::vformat(fmt, std::make_format_args(args...)));
    }
    template<typename... Args>
    void fatal(std::string_view fmt, Args&&... args)
    {
        log(Level::Fatal, std::vformat(fmt, std::make_format_args(args...)));
    }

    Level parseLevel(std::string_view str);
    bool  isEnabled(Level level);
}
