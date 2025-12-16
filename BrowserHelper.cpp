#include "BrowserHelper.h"
#include <tlhelp32.h> //CreateToolhelp32Snapshot, PROCESSENTRY32w
#include <vector>

// PID로부터 프로세스 이름 가져오기
std::wstring BrowserHelper::GetProcessName(DWORD pid) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); //현재 시스템의 모든 프로세스에 대한 스냅샷 획득
	if (hSnapshot == INVALID_HANDLE_VALUE) return L""; //실패 시 빈 문자열 반환

    PROCESSENTRY32W pe = { sizeof(PROCESSENTRY32W) }; //프로세스 정보를 저장할 구조체 선언 및 초기화
    if (Process32FirstW(hSnapshot, &pe)) { //첫번째 프로세스 정보 가져오기
        do {
            if (pe.th32ProcessID == pid) { //찾고자하는 프로세스와 일치하면
                CloseHandle(hSnapshot); //스냅샷 핸들을 닫고 프로세스 이름 반환
                return pe.szExeFile;
            }
		} while (Process32NextW(hSnapshot, &pe)); //다음 프로세스 정보로 이동
    }

    CloseHandle(hSnapshot);
    return L"";
}

// 윈도우 타이틀 가져오기
std::wstring BrowserHelper::GetWindowTitle(HWND hwnd) {
	if (!hwnd) return L""; //유효하지 않은 핸들일 경우 빈 문자열 반환

    int len = GetWindowTextLengthW(hwnd);
    if (len <= 0) return L""; //텍스트가 없으면 빈 문자열 반환

    std::vector<wchar_t> buffer(len + 1); //텍스트 길이 + 널 종단 문자를 위한 벡터 할당
    GetWindowTextW(hwnd, buffer.data(), len + 1); //윈도우 텍스트를 벡터에 복사

    return buffer.data(); //벡터의 내부 데이터를 반환
}

// HWND를 기반으로 브라우저 유형 확인 및 이름 반환
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
    if (_wcsicmp(name.c_str(), L"whale.exe") == 0) return BrowserType::Whale;
    if (_wcsicmp(name.c_str(), L"firefox.exe") == 0) return BrowserType::FireFox;
    if (_wcsicmp(name.c_str(), L"iexplore.exe") == 0) return BrowserType::IE;

    return BrowserType::Unknown;
}

// UIA 루트 윈도우 찾기
HWND BrowserHelper::FindUiaRootWindow(HWND top)
{
    //UIA는 반드시 최상위 윈도우 기준
    return top;
}