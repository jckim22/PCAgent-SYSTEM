#include "CommonUtils.h"
#include <windows.h>
#include <cstdio>
#include <cstdarg>

std::string Utf16ToUtf8(const std::wstring& ws) {
    if (ws.empty()) return std::string();

    // CP_UTF8 상수와 WideCharToMultiByte 사용을 위해 windows.h 필요
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) {
        return std::string();
    }
    std::string result;
    result.resize(static_cast<size_t>(len));
    int written = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &result[0], len, nullptr, nullptr);
    if (written <= 0) {
        return std::string();
    }
    // 결과는 null-terminated 포함. C++ string에서는 마지막 null을 제거해도 무방.
    if (!result.empty() && result.back() == '\0') {
        result.pop_back();
    }
    return result;
}

static void VLog(FILE* fp, const wchar_t* prefix, const wchar_t* fmt, va_list ap) {
    // Windows에서 콘솔에 UTF-16 출력: fwprintf 사용
    fwprintf(fp, L"%ls", prefix);
    vfwprintf(fp, fmt, ap);
    fwprintf(fp, L"\n");
}

void LogInfo(const wchar_t* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    VLog(stdout, L"[INFO] ", fmt, ap);
    va_end(ap);
}

void LogError(const wchar_t* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    VLog(stderr, L"[ERROR] ", fmt, ap);
    va_end(ap);
}