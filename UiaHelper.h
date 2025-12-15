#pragma once
#include <UIAutomation.h>
#include <string>

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

    IUIAutomationElement* FindAddressBarElement(IUIAutomationElement* root);

    bool ReadValueFromElement(IUIAutomationElement* element, std::wstring& valueOut);
    bool ReadTextFromElement(IUIAutomationElement* element, std::wstring& valueOut);
};
