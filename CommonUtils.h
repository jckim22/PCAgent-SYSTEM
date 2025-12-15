#pragma once

#include <string>
#include <vector>
#include <tuple>

// Windows API 변환 유틸리티
// - UTF-16(wstring) -> UTF-8(string)
std::string Utf16ToUtf8(const std::wstring& ws);

// 간단한 로그 함수 (printf 대체)
void LogInfo(const wchar_t* fmt, ...);
void LogError(const wchar_t* fmt, ...);