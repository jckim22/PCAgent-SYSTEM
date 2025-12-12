#pragma once
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include "Database.h"

class WorkerThread
{
public:
    WorkerThread();
    ~WorkerThread();

    void Start();
    void Stop();

    static void PushMessage(std::string msg);

private:
    void ThreadProc();
    void ProcessMessage(const std::string& msg);

    std::thread m_thread;
    std::atomic<bool> m_running{ false };

    static std::queue<std::string> m_queue;
    static std::mutex m_lock;
    static std::condition_variable m_cv;

    Database m_database; // °´Ã¼ ¸â¹ö
};