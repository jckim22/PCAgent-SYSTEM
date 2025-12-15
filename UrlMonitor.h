#pragma once
#include <windows.h>
#include <string>
#include <thread>
#include <atomic>
#include "UiaHelper.h"
#include "Database.h"

class UrlMonitor {
public:
    UrlMonitor(Database* db);
    ~UrlMonitor();

    bool Start();
    void Stop();

private:
    Database* m_database;
    UiaHelper m_uia;

    std::thread m_thread;
    std::atomic<bool> m_running;

    // 중복 저장 방지용
    HWND m_lastHwnd;
    std::wstring m_lastUrl;
    std::wstring m_lastBrowser;

    void MonitorThread();
    void OnUrlChanged(const std::wstring& browser, const std::wstring& url, const std::wstring& title);
};