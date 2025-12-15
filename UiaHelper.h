#pragma once
#include <UIAutomation.h>
#include <string>
#include <unordered_map>
#include <windows.h>

class UiaHelper {
public:
    UiaHelper();
    ~UiaHelper();

    bool Initialize();
    void Shutdown();

    bool GetAddressBarUrl(HWND hwnd, std::wstring& urlOut);

private:
    IUIAutomation* m_uia;
    bool m_initialized;

    // ⭐ HWND → AddressBar 캐시
    std::unordered_map<HWND, IUIAutomationElement*> m_cachedAddr;

    IUIAutomationElement* FindAddressBarElement(IUIAutomationElement* root);

    bool ReadValueFromElement(IUIAutomationElement* element, std::wstring& valueOut);
    bool ReadTextFromElement(IUIAutomationElement* element, std::wstring& valueOut);
};
