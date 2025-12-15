#include "UrlMonitor.h"
#include "BrowserHelper.h"
#include <regex>
#include <stdio.h>

static std::wstring g_candidate;
static int g_count = 0;
static DWORD g_firstTick = 0;

static const std::wregex kUrlRegex(
    LR"(^(https?:\/\/)?([a-z0-9-]+\.)+[a-z]{2,}(:\d+)?(\/.*)?$)",
    std::regex_constants::icase
);

static bool ConfirmUrl(const std::wstring& raw, std::wstring& confirmed)
{
    DWORD now = GetTickCount();

    // 기본 형태 필터
    if (raw.length() < 6) return false;
    if (raw.find(L' ') != std::wstring::npos) return false;

    size_t dot = raw.rfind(L'.');
    if (dot == std::wstring::npos) return false;
    if (raw.length() - dot < 3) return false;

    // 새로운 후보
    if (g_candidate != raw)
    {
        g_candidate = raw;
        g_count = 1;
        g_firstTick = now;
        return false;
    }

    g_count++;

    // ⭐ 3회 연속 + 300ms 유지
    if (g_count >= 3 && now - g_firstTick >= 300)
    {
        std::wstring norm = raw;
        if (norm.find(L"://") == std::wstring::npos)
            norm = L"https://" + norm;

        if (std::regex_match(norm, kUrlRegex))
        {
            confirmed = norm;

            // reset
            g_candidate.clear();
            g_count = 0;
            g_firstTick = 0;

            return true;
        }
    }

    return false;
}
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

        std::wstring raw, confirmed;

        if (m_uia.GetAddressBarUrl(uiaRoot, raw)) {

            if (ConfirmUrl(raw, confirmed)) {

                // ⭐ 동일 URL 중복 방지
                if (uiaRoot != m_lastHwnd || confirmed != m_lastUrl) {

                    std::wstring title = BrowserHelper::GetWindowTitle(uiaRoot);
                    OnUrlChanged(browserName, confirmed, title);

                    m_lastHwnd = uiaRoot;
                    m_lastUrl = confirmed;
                    m_lastBrowser = browserName;
                }
            }
        }

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