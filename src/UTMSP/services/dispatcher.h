/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <iostream>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>

class Dispatcher
{
public:
    Dispatcher() : _running(true)
    {
        _workerThread = std::thread([this]() {
            while (_running) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(_queueMutex);
                    _conditionVariable.wait(lock, [this] { return !_tasks.empty() || !_running; });
                    if (!_running && _tasks.empty()) {
                        return;
                    }
                    task = std::move(_tasks.front());
                    _tasks.pop_front();
                }
                task();
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });
    }

    ~Dispatcher()
    {
        std::cout<<" DESTRUCTOR is the problem "<<std::endl;
        {
            std::unique_lock<std::mutex> lock(_queueMutex);
            _running = false;
        }
        _conditionVariable.notify_all();
        _workerThread.join();
    }

    void add_task(std::function<void()> task)
    {
        {
            std::unique_lock<std::mutex> lock(_queueMutex);
            _tasks.push_back(std::move(task));
        }
        _conditionVariable.notify_one();
    }

private:
    std::deque<std::function<void()>>   _tasks;
    std::mutex                          _queueMutex;
    std::condition_variable             _conditionVariable;
    std::thread                         _workerThread;
    bool                                _running;
};
