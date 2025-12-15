#pragma once
#include <windows.h>
#include <string>

// 브라우저 유형 정의
enum class BrowserType {
    Unknown,
    Chrome,   // Chrome, Edge (Chromium 기반)
    Edge,
    FireFox,
    Whale,
    IE        // Internet Explorer
};

class BrowserHelper {
public:
    // PID로부터 프로세스 이름 가져오기
    static std::wstring GetProcessName(DWORD pid);

    // 윈도우 타이틀 가져오기
    static std::wstring GetWindowTitle(HWND hwnd);

    // HWND를 기반으로 대상 브라우저인지 확인하고 유형과 이름을 반환
    static BrowserType GetBrowserType(HWND hwnd, std::wstring& exeNameOut);

    // UIA 루트 윈도우 찾기
    static HWND FindUiaRootWindow(HWND top);
};