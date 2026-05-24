/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#include "AsyncQueryPool.h"
#include "Shared/Server/Log/Logger.h"

AsyncQueryPool::AsyncQueryPool(Database* db) : _db(db)
{
    // Start worker threads
    for (size_t i = 0; i < THREAD_COUNT; ++i)
        _threads.emplace_back(&AsyncQueryPool::worker, this);

    Log::info("AsyncQueryPool: started {} worker threads", THREAD_COUNT);
}

AsyncQueryPool::~AsyncQueryPool()
{
    _stopping = true;
    _cv.notify_all();

    for (auto& t : _threads)
        if (t.joinable())
            t.join();

    Log::info("AsyncQueryPool: stopped");
}

UInt32 AsyncQueryPool::submit(const std::string& sql)
{
    UInt32 token = ++_tokenCounter;

    // Reserve slot in results
    {
        std::unique_lock<std::mutex> lock(_resultsMutex);
        _results[token] = AsyncQueryResult{};
    }

    // Push job to queue
    {
        std::unique_lock<std::mutex> lock(_queueMutex);
        _queue.push_back({ token, sql });
    }
    _cv.notify_one();

    Log::trace("AsyncQueryPool: submitted token={} sql={}", token, sql);
    return token;
}

int AsyncQueryPool::status(UInt32 token)
{
    std::unique_lock<std::mutex> lock(_resultsMutex);

    auto it = _results.find(token);
    if (it == _results.end())
        return -1;
    if (!it->second.ready)
        return 0;
    if (!it->second.error.empty())
        return -1;
    return 1;
}

std::optional<std::vector<std::string>> AsyncQueryPool::fetchRow(UInt32 token)
{
    std::unique_lock<std::mutex> lock(_resultsMutex);

    auto it = _results.find(token);
    if (it == _results.end() || !it->second.ready)
        return std::nullopt;

    auto& qr = it->second;
    if (qr.currentRow >= qr.rows.size())
        return std::vector<std::string>{};

    return qr.rows[qr.currentRow++];
}

std::optional<std::vector<std::string>> AsyncQueryPool::fieldNames(UInt32 token)
{
    std::unique_lock<std::mutex> lock(_resultsMutex);

    auto it = _results.find(token);
    if (it == _results.end() || !it->second.ready)
        return std::nullopt;

    return it->second.fieldNames;
}

bool AsyncQueryPool::close(UInt32 token)
{
    std::unique_lock<std::mutex> lock(_resultsMutex);
    bool erased = _results.erase(token) > 0;

    if (erased)
        Log::trace("AsyncQueryPool: closed token={}", token);
    else
        Log::warning("AsyncQueryPool: close unknown token={}", token);

    return erased;
}

void AsyncQueryPool::worker()
{
    while (true)
    {
        Job job;
        {
            std::unique_lock<std::mutex> lock(_queueMutex);
            _cv.wait(lock, [this] { return _stopping || !_queue.empty(); });
            if (_stopping && _queue.empty())
                return;
            job = std::move(_queue.front());
            _queue.pop_front();
        }
        Log::trace("AsyncQueryPool: executing token={}", job.token);
        AsyncQueryResult result;
        result.ready = true;
        try
        {
            auto qr = _db->query(job.sql.c_str());
            if (!qr)
            {
                result.error = "Query returned null";
            }
            else
            {
                result.fieldNames = qr->fetchFieldNames();
                while (qr->fetchRow())
                {
                    std::vector<std::string> row;
                    for (size_t i = 0; i < qr->numFields(); ++i)
                        row.push_back(qr->at(i).getString());
                    result.rows.push_back(std::move(row));
                }
            }
        }
        catch (const std::exception& e)
        {
            result.error = e.what();
            Log::error("AsyncQueryPool: token={} exception: {}", job.token, e.what());
        }
        catch (...)
        {
            result.error = "Unknown exception";
            Log::error("AsyncQueryPool: token={} unknown exception", job.token);
        }
        {
            std::unique_lock<std::mutex> lock(_resultsMutex);
            _results[job.token] = std::move(result);
        }
        Log::trace("AsyncQueryPool: token={} done", job.token);
    }
}
