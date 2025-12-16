#include "UrlMonitor.h"
#include "BrowserHelper.h"
#include "CommonUtils.h" // Utf16ToUtf8 사용
#include <regex>
#include <stdio.h>
#include <string>
#include "madCHook.h" // SendIpcMessage 사용

// IPC 정의 (IpcServer.cpp와 동일하게 정의)
#define IPC_NAME_URL "BrowserUrlEvent"
#define IMT_URL_EVENT 0x9001

#pragma pack(push,1)
typedef struct _IPC_URL_MSG_HEADER {
    DWORD nType; DWORD dwSize;
} IPC_URL_MSG_HEADER, * PIPC_URL_MSG_HEADER;
#pragma pack(pop)

static std::wstring g_candidate; //URL 의심 후보
static int g_count = 0; //연속 확인 횟수
static DWORD g_firstTick = 0; //URL 후보 처음 나타난 시점 (URL이 너무 빨리 바뀌는 경우 방지용)

static const std::wregex kUrlRegex(
    LR"(^(https?:\/\/)?([a-z0-9-]+\.)+[a-z]{2,}(:\d+)?(\/.*)?$)",
    std::regex_constants::icase
); //URL 형식 검사용 정규식 (불변)

// 확정 로직 기준: 2회 연속, 100ms
static bool ConfirmUrl(const std::wstring& raw, std::wstring& confirmed)
{
    DWORD now = GetTickCount();

	// 기본 형태 검사(길이, 공백, 최소 도메인 등)
    if (raw.length() < 6) return false;
    if (raw.find(L' ') != std::wstring::npos) return false;

    size_t dot = raw.rfind(L'.');
    if (dot == std::wstring::npos) return false;
    if (raw.length() - dot < 3) return false;

    // 새로운 후보가 들어오면 카운트 초기화
    if (g_candidate != raw) 
    {
        g_candidate = raw;
        g_count = 1;
        g_firstTick = now;
        return false;
    }

    g_count++;

    // 확정 조건: 2회 연속 + 100ms
    if (g_count >= 2 && now - g_firstTick >= 100)
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
        return false;
    }

	m_thread = std::thread(&UrlMonitor::MonitorThread, this); //URL 모니터링 스레드 시작
    printf("[UrlMonitor] Started\n");
    return true;
}

void UrlMonitor::Stop() {
	m_running.store(false); //스레드 루프 종료 신호
    if (m_thread.joinable()) { //스레드가 실행중이면 종료될 때까지 대기
        m_thread.join();
    }
    m_uia.Shutdown(); //URL 모니터링 UIA 자원 해제
    printf("[UrlMonitor] Stopped\n");
}

void UrlMonitor::MonitorThread() {
    // 스레드별 COM 초기화 (UIA 사용을 위해 필수)
    HRESULT hrCo = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool comInitialized = (SUCCEEDED(hrCo) || hrCo == RPC_E_CHANGED_MODE);

    if (!m_uia.Initialize()) {
        printf("[UrlMonitor] UIA init failed in worker thread\n");
        if (comInitialized) CoUninitialize();
        return;
    }


	while (m_running.load()) { //m_running이 true인 동안 반복
		HWND fg = GetForegroundWindow(); //현재 포그라운드 윈도우 핸들 가져오기
        if (!fg) { Sleep(200); continue; }

		HWND top = GetAncestor(fg, GA_ROOT); //최상위 윈도우 핸들 가져오기
        if (!top) { Sleep(200); continue; }

        HWND uiaRoot = BrowserHelper::FindUiaRootWindow(top); //UIA 지원 브라우저 윈도우 찾기
        if (!uiaRoot) {
            Sleep(200);
            continue;
        }

        std::wstring browserName;
        // 브라우저 윈도우인지 확인 및 브라우저 유형 획득
        BrowserType type = BrowserHelper::GetBrowserType(uiaRoot, browserName);
        if (type == BrowserType::Unknown) {
            Sleep(200);
            continue;
        }

        std::wstring raw, confirmed;

        // UIA 호출 시 브라우저 유형 전달 (유형별 로직 분기)
        if (m_uia.GetAddressBarUrl(uiaRoot, type, raw)) { //UIA를 통해 주소 표시줄의 URL 후보를 읽어옴

            if (ConfirmUrl(raw, confirmed)) { //URL 확정 로직 수행

                // 동일 URL 중복 방지
                if (uiaRoot != m_lastHwnd || confirmed != m_lastUrl) {

					std::wstring title = BrowserHelper::GetWindowTitle(uiaRoot); //윈도우 타이틀 가져오기
					OnUrlChanged(browserName, confirmed, title); //URL 변경 이벤트 처리

                    m_lastHwnd = uiaRoot;
                    m_lastUrl = confirmed;
                    m_lastBrowser = browserName;
                }
            }
        }

        Sleep(200); // 모니터링 주기
    }

    m_uia.Shutdown();
    if (comInitialized) CoUninitialize();
}

//URL 확정 시 데이터베이스 저장 및 IPC 메시지 전송
void UrlMonitor::OnUrlChanged(const std::wstring& browser, const std::wstring& url, const std::wstring& title) {
    printf("[UrlMonitor] %ls: %ls\n", browser.c_str(), url.c_str());

    if (m_database) {
        m_database->SaveBrowserUrl(browser, url, title);
    }

    // IPC 메시지 전송 로직
    // 데이터 형식: [BrowserName]|[URL]|[WindowTitle]
    std::wstring wpayload = browser + L"|" + url + L"|" + title;
    std::string payload = Utf16ToUtf8(wpayload);

    DWORD payloadSize = (DWORD)payload.size() + 1; // null-terminator 포함
    DWORD totalSize = sizeof(IPC_URL_MSG_HEADER) + payloadSize;

    PIPC_URL_MSG_HEADER hdr = (PIPC_URL_MSG_HEADER)malloc(totalSize);
    if (!hdr) return;

    memset(hdr, 0, totalSize);

    hdr->nType = IMT_URL_EVENT;
    hdr->dwSize = payloadSize;

    // 페이로드 복사
    memcpy((BYTE*)hdr + sizeof(IPC_URL_MSG_HEADER),
        payload.c_str(), payloadSize);

    // SendIpcMessage는 madCHook에 정의된 함수
    BOOL ok = SendIpcMessage(IPC_NAME_URL, hdr, totalSize);
    if (!ok) {
        printf("[UrlMonitor] Failed to send URL IPC message to user program\n");
    }

    free(hdr);
}