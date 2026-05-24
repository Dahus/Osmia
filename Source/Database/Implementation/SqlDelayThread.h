/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>

class Database;
class SqlOperation;
class SqlConnection;

class SqlDelayThread
{
public:
    SqlDelayThread(Database& db, SqlConnection& conn);
    virtual ~SqlDelayThread();

    bool queueOperation(std::unique_ptr<SqlOperation> op);
    virtual void stop();

    // Called by std::thread — not Poco::Runnable
    void run();

    SqlDelayThread(const SqlDelayThread&) = delete;
    SqlDelayThread& operator=(const SqlDelayThread&) = delete;

protected:
    virtual void processRequests();

    Database& _db;
    SqlConnection& _conn;

private:
    std::queue<std::unique_ptr<SqlOperation>> _queue;
    std::mutex                _mutex;
    std::condition_variable   _cv;
    std::atomic<bool>         _running{ false };
};
