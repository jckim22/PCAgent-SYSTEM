#include "BrowserHelper.h"
#include <tlhelp32.h>
#include <vector>

bool BrowserHelper::IsTargetBrowser(HWND hwnd, std::wstring& exeNameOut) {
    if (!hwnd) return false;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    std::wstring name = GetProcessName(pid);
    if (name.empty()) return false;

    exeNameOut = name;

    // 대소문자 구분 없이 비교
    if (_wcsicmp(name.c_str(), L"chrome.exe") == 0) return true;
    if (_wcsicmp(name.c_str(), L"msedge.exe") == 0) return true;
    // 필요 시 firefox.exe, brave.exe 등 추가

    return false;
}

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

std::wstring BrowserHelper::GetWindowTitle(HWND hwnd) {
    if (!hwnd) return L"";

    int len = GetWindowTextLengthW(hwnd);
    if (len <= 0) return L"";

    std::vector<wchar_t> buffer(len + 1);
    GetWindowTextW(hwnd, buffer.data(), len + 1);

    return buffer.data();
}

HWND BrowserHelper::FindUiaRootWindow(HWND top)
{
    // ❗ UIA는 반드시 최상위 윈도우 기준
    return top;
}