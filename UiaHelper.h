#pragma once
#include <UIAutomation.h>
#include <string>
#include <unordered_map>
#include <windows.h>
#include "BrowserHelper.h" // BrowserType 사용을 위해 포함

class UiaHelper {
public:
    UiaHelper();
    ~UiaHelper();

    bool Initialize();
    void Shutdown();

    // 브라우저 유형을 인자로 받도록 수정
    bool GetAddressBarUrl(HWND hwnd, BrowserType type, std::wstring& urlOut);

private:
    IUIAutomation* m_uia;
    bool m_initialized;

    // HWND → AddressBar 캐시
    std::unordered_map<HWND, IUIAutomationElement*> m_cachedAddr;

    // 브라우저 유형별 주소 표시줄 요소를 찾는 함수로 변경
    IUIAutomationElement* FindAddressBarElementByBrowser(
        IUIAutomationElement* root, BrowserType type);

    // UIA 패턴을 이용해 값 읽는 함수들 (변화 없음)
    bool ReadValueFromElement(IUIAutomationElement* element, std::wstring& valueOut);
    bool ReadTextFromElement(IUIAutomationElement* element, std::wstring& valueOut);
};