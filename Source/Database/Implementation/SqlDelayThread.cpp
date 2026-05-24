/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#include "SqlDelayThread.h"
#include "SqlOperations.h"

SqlDelayThread::SqlDelayThread(Database& db, SqlConnection& conn)
    : _db(db), _conn(conn), _running(true) {}

SqlDelayThread::~SqlDelayThread()
{
    stop();
    processRequests();
}

bool SqlDelayThread::queueOperation(std::unique_ptr<SqlOperation> op)
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push(std::move(op));
    }
    _cv.notify_one();
    return true;
}

void SqlDelayThread::stop()
{
    _running = false;
    _cv.notify_all();
}

void SqlDelayThread::run()
{
    while (_running)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _cv.wait_for(lock, std::chrono::milliseconds(10),
            [this] { return !_queue.empty() || !_running; });
        lock.unlock();
        processRequests();
    }
    processRequests();
}

void SqlDelayThread::processRequests()
{
    while (true)
    {
        std::unique_ptr<SqlOperation> op;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_queue.empty()) break;
            op = std::move(_queue.front());
            _queue.pop();
        }
        op->execute(_conn);
    }
}
