#include "BrowserHelper.h"
#include <tlhelp32.h>
#include <vector>

// PID로부터 프로세스 이름 가져오기 (이전과 동일)
std::wstring BrowserHelper::GetProcessName(DWORD pid) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return L"";

    PROCESSENTRY32W pe = { sizeof(PROCESSENTRY32W) };
    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            if (pe.th32ProcessID == pid) {
                CloseHandle(hSnapshot);
                return pe.szExeFile;
            }
        } while (Process32NextW(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    return L"";
}

// 윈도우 타이틀 가져오기 (이전과 동일)
std::wstring BrowserHelper::GetWindowTitle(HWND hwnd) {
    if (!hwnd) return L"";

    int len = GetWindowTextLengthW(hwnd);
    if (len <= 0) return L"";

    std::vector<wchar_t> buffer(len + 1);
    GetWindowTextW(hwnd, buffer.data(), len + 1);

    return buffer.data();
}

// HWND를 기반으로 브라우저 유형 확인 및 이름 반환 (업그레이드된 로직)
BrowserType BrowserHelper::GetBrowserType(HWND hwnd, std::wstring& exeNameOut) {
    if (!hwnd) return BrowserType::Unknown;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid); // HWND로부터 PID 얻기

    std::wstring name = GetProcessName(pid); // PID로부터 프로세스 이름 얻기
    if (name.empty()) return BrowserType::Unknown;

    exeNameOut = name;

    // 대소문자 구분 없이 비교 (_wcsicmp)
    if (_wcsicmp(name.c_str(), L"chrome.exe") == 0) return BrowserType::Chrome;
    if (_wcsicmp(name.c_str(), L"msedge.exe") == 0) return BrowserType::Edge;
    if (_wcsicmp(name.c_str(), L"whale.exe") == 0) return BrowserType::Whale; // 네이버 웨일 추가
    if (_wcsicmp(name.c_str(), L"firefox.exe") == 0) return BrowserType::FireFox; // 파이어폭스 추가
    if (_wcsicmp(name.c_str(), L"iexplore.exe") == 0) return BrowserType::IE; // Internet Explorer 추가

    return BrowserType::Unknown;
}

// UIA 루트 윈도우 찾기 (이전과 동일)
HWND BrowserHelper::FindUiaRootWindow(HWND top)
{
    // ❗ UIA는 반드시 최상위 윈도우 기준
    return top;
}