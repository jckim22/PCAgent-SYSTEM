#include <windows.h>
#include "WorkerThread.h"
#include <stdio.h>
#include <sstream>
#include "madCHook.h"

// static 멤버 초기화
std::queue<std::string> WorkerThread::m_queue;
std::queue<std::string> WorkerThread::m_urlQueue;
std::mutex WorkerThread::m_lock;
std::mutex WorkerThread::m_urlLock;
std::condition_variable WorkerThread::m_cv;
std::condition_variable WorkerThread::m_urlCv;

WorkerThread::WorkerThread() : m_running(false) {
}

WorkerThread::~WorkerThread() {
    Stop();
}

void WorkerThread::Start() {
    bool expected = false;
    if (!m_running.compare_exchange_strong(expected, true)) {
        return; // 이미 실행 중
    }

    if (!m_database.Initialize("C:\\ProgramData\\AgentOptions.db")) {
        printf("[SYSTEM] DB init failed\n");
        m_running.store(false);
        return;
    }

    m_thread = std::thread(&WorkerThread::ThreadProc, this);
    m_urlThread = std::thread(&WorkerThread::UrlThreadProc, this);
    printf("[SYSTEM] WorkerThread started\n");
}

void WorkerThread::Stop() {
    m_running.store(false);
    m_cv.notify_all();
    m_urlCv.notify_all();

    if (m_thread.joinable()) {
        m_thread.join();
    }
    if (m_urlThread.joinable()) {
        m_urlThread.join();
    }

    m_database.Close();
    printf("[SYSTEM] WorkerThread stopped\n");
}

void WorkerThread::PushMessage(std::string msg) {
    {
        std::lock_guard<std::mutex> guard(m_lock);
        m_queue.push(std::move(msg));
    }
    m_cv.notify_one();
}

void WorkerThread::PushUrlMessage(std::string msg) {
    {
        std::lock_guard<std::mutex> guard(m_urlLock);
        m_urlQueue.push(std::move(msg));
    }
    m_urlCv.notify_one();
}

void WorkerThread::ThreadProc() {
    while (true) {
        std::unique_lock<std::mutex> lk(m_lock);
        m_cv.wait(lk, [this]() {
            return !m_queue.empty() || !m_running.load();
            });

        if (!m_queue.empty()) {
            std::string msg = std::move(m_queue.front());
            m_queue.pop();
            lk.unlock();

            ProcessMessage(msg);
        }
        else if (!m_running.load()) {
            break;
        }
    }
}

void WorkerThread::UrlThreadProc() {
    while (true) {
        std::unique_lock<std::mutex> lk(m_urlLock);
        m_urlCv.wait(lk, [this]() {
            return !m_urlQueue.empty() || !m_running.load();
            });

        if (!m_urlQueue.empty()) {
            std::string msg = std::move(m_urlQueue.front());
            m_urlQueue.pop();
            lk.unlock();

            ProcessUrlMessage(msg);
        }
        else if (!m_running.load()) {
            break;
        }
    }
}

void WorkerThread::ProcessMessage(const std::string& msg) {
    printf("[SYSTEM] Worker Process msg: %s\n", msg.c_str());

    int seq = 0;
    int opt1 = 0, opt2 = 0, opt3 = 0;

    std::istringstream ss(msg);
    std::string token;

    while (std::getline(ss, token, ';')) {
        size_t pos = token.find('=');
        if (pos != std::string::npos) {
            std::string key = token.substr(0, pos);
            int value = std::stoi(token.substr(pos + 1));

            if (key == "OPT1") opt1 = value;
            else if (key == "OPT2") opt2 = value;
            else if (key == "OPT3") opt3 = value;
            else if (key == "SEQ") seq = value;
        }
    }

    bool success = m_database.SaveOptions(seq, opt1, opt2, opt3);

    std::string response = success
        ? ("SYSTEM: 적용되었습니다. DB 저장 완료 (OPT1=" + std::to_string(opt1) +
            ", OPT2=" + std::to_string(opt2) + ", OPT3=" + std::to_string(opt3) + ")")
        : "SYSTEM: 저장 실패. DB 오류";

    const char* userQueue = "UserOptionResponse";
    BOOL ok = SendIpcMessage(userQueue, (void*)response.c_str(), (DWORD)response.size() + 1);

    if (ok) {
        printf("[SYSTEM] Sent response: %s\n", response.c_str());
    }
    else {
        printf("[SYSTEM] Failed to send response\n");
    }
}

void WorkerThread::ProcessUrlMessage(const std::string& msg) {
    printf("[SYSTEM] URL message received: %s\n", msg.c_str());    
}