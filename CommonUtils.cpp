#include "CommonUtils.h"
#include <windows.h>
#include <cstdio>
#include <cstdarg>

//Win32 API와 C++ 표준 라이브러리 간의 문자열 인코딩 기준 호환
std::string Utf16ToUtf8(const std::wstring& ws) {
    if (ws.empty()) return std::string(); //입력 문자열 비어있으면 빈 문자열 반환 

    // CP_UTF8 상수와 WideCharToMultiByte 사용을 위해 windows.h 필요
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr); //문자열 담을 필요 버퍼 크기 계산
    if (len <= 0) {
		return std::string(); //버퍼 크기 계산 실패 시 빈 문자열 반환
    }
	std::string result; //UTF-8 결과 문자열
    result.resize(static_cast<size_t>(len)); //len 만큼 문자열의 크기 조정(메모리 할당)
    int written = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &result[0], len, nullptr, nullptr); //UTF-8 데이터 쓰기
    if (written <= 0) {
        return std::string(); //실패 시 빈 문자열 반환
    }
    // 결과는 null-terminated 포함. C++ string에서는 마지막 null을 제거해도 무방.
    if (!result.empty() && result.back() == '\0') {
		result.pop_back(); //마지막 null 문자 제거
    }
    return result; //최종 변환 문자열 반환
}

//로그 출력(가변)
static void VLog(FILE* fp, const wchar_t* prefix, const wchar_t* fmt, va_list ap) {
    // Windows에서 콘솔에 UTF-16 출력: fwprintf 사용
    fwprintf(fp, L"%ls", prefix);
    vfwprintf(fp, fmt, ap); //로그 메시지 본문 출력
    fwprintf(fp, L"\n");
}

//정보 로그 출력
void LogInfo(const wchar_t* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    VLog(stdout, L"[INFO] ", fmt, ap); // 표준 출력
    va_end(ap);
}

//에러 로그 출력
void LogError(const wchar_t* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    VLog(stderr, L"[ERROR] ", fmt, ap);
    va_end(ap);
}