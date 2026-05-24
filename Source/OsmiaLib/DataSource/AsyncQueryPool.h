/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "Shared/Common/Types.h"
#include "Database/Database.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <unordered_map>
#include <vector>
#include <deque>
#include <string>
#include <functional>
#include <optional>

 // Represents a completed query result stored in memory
struct AsyncQueryResult
{
    std::vector<std::string>              fieldNames;
    std::vector<std::vector<std::string>> rows;
    size_t                                currentRow = 0;
    bool                                  ready = false;
    std::string                           error;      // non-empty if query failed
};

// Manages async query execution and result retrieval
// Tokens are 32-bit handles handed to Lua, results stored until closed
class AsyncQueryPool
{
public:
    explicit AsyncQueryPool(Database* db);
    ~AsyncQueryPool();

    // Submit a query for async execution, returns token
    UInt32 submit(const std::string& sql);

    // Check if query is ready: 0=pending, 1=ready, -1=error
    int status(UInt32 token);

    // Fetch next row, returns empty vector if no more rows
    // Returns nullopt if token unknown or not ready
    std::optional<std::vector<std::string>> fetchRow(UInt32 token);

    // Get field names for a completed query
    std::optional<std::vector<std::string>> fieldNames(UInt32 token);

    // Close and discard query result
    bool close(UInt32 token);

private:
    // Worker function executed in a thread pool thread
    void worker();

    // Pending job: token + sql to execute
    struct Job
    {
        UInt32      token;
        std::string sql;
    };

    Database* _db;
    std::unordered_map<UInt32, AsyncQueryResult>   _results;
    std::mutex                                     _resultsMutex;
    std::deque<Job>                                _queue;
    std::mutex                                     _queueMutex;
    std::vector<std::thread>                       _threads;
    std::condition_variable                        _cv;
    std::atomic<bool>                              _stopping{ false };
    std::atomic<UInt32>                            _tokenCounter{ 0 };

    static constexpr size_t THREAD_COUNT = 2;
};
