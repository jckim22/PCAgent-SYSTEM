#pragma once
#include <windows.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <string>
#include <atomic>
#include "Database.h"

class WorkerThread {
public:
    WorkerThread();
    ~WorkerThread();

    void Start();
    void Stop();
    void PushMessage(std::string msg);
    void PushUrlMessage(std::string msg); // URL 메시지용

    // Database 포인터 반환
    Database* GetDatabase() { return &m_database; }

private:
    Database m_database;

    std::thread m_thread;
    std::thread m_urlThread; // URL 처리용 별도 스레드
    std::atomic<bool> m_running;

    static std::queue<std::string> m_queue;
    static std::queue<std::string> m_urlQueue; // URL 전용 큐
    static std::mutex m_lock;
    static std::mutex m_urlLock; // URL 큐용 뮤텍스
    static std::condition_variable m_cv;
    static std::condition_variable m_urlCv; // URL 큐용 cv

    void ThreadProc();
    void UrlThreadProc(); // URL 처리 스레드
    void ProcessMessage(const std::string& msg);
    void ProcessUrlMessage(const std::string& msg); // URL 메시지 처리
};