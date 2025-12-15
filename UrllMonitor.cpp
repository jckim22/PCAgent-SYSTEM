#include "UrlMonitor.h"
#include "BrowserHelper.h"
#include <stdio.h>

UrlMonitor::UrlMonitor(Database* db)
    : m_database(db)
    , m_running(false)
    , m_lastHwnd(nullptr)
{
}

UrlMonitor::~UrlMonitor() {
    Stop();
}

bool UrlMonitor::Start() {
    bool expected = false;
    if (!m_running.compare_exchange_strong(expected, true)) {
        return false; // 이미 실행 중
    }

    if (!m_uia.Initialize()) {
        printf("[UrlMonitor] UIA init failed\n");
        m_running.store(false);
        return false;
    }

    m_thread = std::thread(&UrlMonitor::MonitorThread, this);
    printf("[UrlMonitor] Started\n");
    return true;
}

void UrlMonitor::Stop() {
    m_running.store(false);
    if (m_thread.joinable()) {
        m_thread.join();
    }
    m_uia.Shutdown();
    printf("[UrlMonitor] Stopped\n");
}

void UrlMonitor::MonitorThread() {
    // 스레드별 COM 초기화
    HRESULT hrCo = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool comInitialized = (SUCCEEDED(hrCo) || hrCo == RPC_E_CHANGED_MODE);
    
    if (!m_uia.Initialize()) {
        printf("[UrlMonitor] UIA init failed in worker thread\n");
        if (comInitialized) CoUninitialize();
        return;
    }

    // UIA 타임아웃 설정(필요 시)
    // m_uia.SetTimeout(2000); // UiaHelper에 메서드 추가로 구현

    while (m_running.load()) {
        HWND fg = GetForegroundWindow();
        if (!fg) { Sleep(500); continue; }

        // 최상위 윈도우로 승격
        HWND top = GetAncestor(fg, GA_ROOT);
        if (!top) { Sleep(500); continue; }

        HWND uiaRoot = BrowserHelper::FindUiaRootWindow(top);
        if (!uiaRoot) {
            Sleep(500);
            continue;
        }


        // 브라우저 윈도우인지 확인
        std::wstring browserName;
        if (!BrowserHelper::IsTargetBrowser(uiaRoot, browserName)) {
            Sleep(500);
            continue;
        }

        // URL 읽기
        std::wstring url;
        if (!m_uia.GetAddressBarUrl(uiaRoot, url)) {
            Sleep(500);
            continue;
        }

        // 변경 감지 (동일 윈도우 + 동일 URL이면 스킵)
        if (uiaRoot == m_lastHwnd && url == m_lastUrl && browserName == m_lastBrowser) {
            Sleep(500);
            continue;
        }

        // 새로운 URL 발견
        std::wstring title = BrowserHelper::GetWindowTitle(uiaRoot);
        OnUrlChanged(browserName, url, title);

        m_lastHwnd = uiaRoot;
        m_lastUrl = url;
        m_lastBrowser = browserName;

        Sleep(500); // 주기 조정 가능
    }

    m_uia.Shutdown();
    if (comInitialized) CoUninitialize();
}

void UrlMonitor::OnUrlChanged(const std::wstring& browser, const std::wstring& url, const std::wstring& title) {
    printf("[UrlMonitor] %ls: %ls\n", browser.c_str(), url.c_str());

    if (m_database) {
        m_database->SaveBrowserUrl(browser, url, title);
    }

    // 필요 시 USER 프로그램에 알림 전송 (IPC)
    // SendIpcMessage("BrowserUrlChanged", ...);
}