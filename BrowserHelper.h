#pragma once
#include <windows.h>
#include <string>

class BrowserHelper {
public:
    // 대상 브라우저인지 확인 (chrome.exe, msedge.exe 등)
    static bool IsTargetBrowser(HWND hwnd, std::wstring& exeNameOut);

    // PID로부터 프로세스 이름 가져오기
    static std::wstring GetProcessName(DWORD pid);

    // 윈도우 타이틀 가져오기
    static std::wstring GetWindowTitle(HWND hwnd);
    static HWND FindUiaRootWindow(HWND top);
};