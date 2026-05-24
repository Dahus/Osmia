/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#include "Logger.h"
#include "IConsoleChannel.h"
#include <fstream>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <iostream>
#include <array>

namespace Log
{
    static constexpr std::array<std::string_view, 7> kLevelNames =
    {
        "TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL", "NONE"
    };

    static std::string_view levelName(Level level)
    {
        auto idx = static_cast<size_t>(level);
        return idx < kLevelNames.size() ? kLevelNames[idx] : "UNKNOWN";
    }

    static std::string currentTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &time);
#else
        localtime_r(&time, &tm);
#endif
        return std::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}",
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec);
    }

    struct LogEntry
    {
        Level       level;
        std::string message;
        std::string timestamp;
    };

    struct LoggerState
    {
        std::ofstream   fileStream;
        Level           fileLevel = Level::Info;
        Level           consoleLevel = Level::Info;
        bool            async = false;

        std::unique_ptr<IConsoleChannel> consoleChannel;

        std::mutex writeMutex;

        // Async
        std::thread             workerThread;
        std::queue<LogEntry>    queue;
        std::mutex              queueMutex;
        std::condition_variable cv;
        std::atomic<bool>       running{ false };
    };

    static LoggerState gState;

    static void writeEntry(const LogEntry& entry)
    {
        std::string line = std::format("[{}] [{}] {}\n",
            entry.timestamp,
            levelName(entry.level),
            entry.message);

        // File output
        if (entry.level >= gState.fileLevel && gState.fileLevel != Level::None && gState.fileStream.is_open())
            gState.fileStream << line << std::flush;

        // Console output
        if (entry.level >= gState.consoleLevel && gState.consoleLevel != Level::None)
        {
            if (gState.consoleChannel)
                gState.consoleChannel->log(entry.level, entry.message);
            else
                (entry.level >= Level::Error ? std::cerr : std::cout) << line;
        }
    }

    static void workerLoop()
    {
        while (gState.running.load())
        {
            std::unique_lock<std::mutex> lock(gState.queueMutex);
            gState.cv.wait(lock, [] {
                return !gState.queue.empty() || !gState.running.load();
            });

            while (!gState.queue.empty())
            {
                LogEntry entry = std::move(gState.queue.front());
                gState.queue.pop();
                lock.unlock();
                writeEntry(entry);
                lock.lock();
            }
        }

        // Drain remaining entries on shutdown
        std::lock_guard<std::mutex> lock(gState.queueMutex);
        while (!gState.queue.empty())
        {
            writeEntry(gState.queue.front());
            gState.queue.pop();
        }
    }

    void init(std::string_view logFilePath, Level fileLevel, Level consoleLevel, bool async)
    {
        gState.fileLevel = fileLevel;
        gState.consoleLevel = consoleLevel;
        gState.async = async;

        gState.fileStream.open(std::string(logFilePath), std::ios::app);
        if (!gState.fileStream.is_open())
            std::cerr << std::format("Logger: failed to open log file: {}\n", logFilePath);

        if (async)
        {
            gState.running.store(true);
            gState.workerThread = std::thread(workerLoop);
        }
    }

    void shutdown()
    {
        if (gState.async)
        {
            gState.running.store(false);
            gState.cv.notify_all();
            if (gState.workerThread.joinable())
                gState.workerThread.join();
        }

        if (gState.fileStream.is_open())
            gState.fileStream.close();

        gState.consoleChannel.reset();
    }

    void setConsoleChannel(std::unique_ptr<IConsoleChannel> channel)
    {
        std::lock_guard<std::mutex> lock(gState.writeMutex);
        gState.consoleChannel = std::move(channel);
    }

    void setFileLevel(Level level) { gState.fileLevel = level; }
    void setConsoleLevel(Level level) { gState.consoleLevel = level; }

    bool isEnabled(Level level)
    {
        if (level == Level::None) return false;
        return level >= gState.fileLevel || level >= gState.consoleLevel;
    }

    void log(Level level, std::string_view msg)
    {
        if (!isEnabled(level)) return;

        LogEntry entry
        {
            .level = level,
            .message = std::string(msg),
            .timestamp = currentTimestamp()
        };

        if (gState.async)
        {
            {
                std::lock_guard<std::mutex> lock(gState.queueMutex);
                gState.queue.push(std::move(entry));
            }
            gState.cv.notify_one();
        }
        else
        {
            std::lock_guard<std::mutex> lock(gState.writeMutex);
            writeEntry(entry);
        }
    }

    Level parseLevel(std::string_view str)
    {
        if (str == "trace")   return Level::Trace;
        if (str == "debug")   return Level::Debug;
        if (str == "info")    return Level::Info;
        if (str == "warning") return Level::Warning;
        if (str == "error")   return Level::Error;
        if (str == "fatal")   return Level::Fatal;
        if (str == "none")    return Level::None;

        return Level::Info;
    }
}
