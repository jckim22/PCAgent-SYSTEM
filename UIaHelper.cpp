#include "UiaHelper.h"
#include <vector>
#include <stdio.h>

UiaHelper::UiaHelper() : m_uia(nullptr), m_initialized(false) {}
UiaHelper::~UiaHelper() { Shutdown(); }

bool UiaHelper::Initialize() {
    if (m_initialized) return true;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        printf("[UIA] CoInitialize failed: 0x%08X\n", hr);
        return false;
    }

    hr = CoCreateInstance(
        CLSID_CUIAutomation,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_IUIAutomation,
        (void**)&m_uia
    );

    if (FAILED(hr) || !m_uia) {
        printf("[UIA] CoCreateInstance failed: 0x%08X\n", hr);
        CoUninitialize();
        return false;
    }

    m_initialized = true;
    printf("[UIA] Initialized\n");
    return true;
}

void UiaHelper::Shutdown() {
    if (m_uia) {
        m_uia->Release();
        m_uia = nullptr;
    }
    if (m_initialized) {
        CoUninitialize();
        m_initialized = false;
    }
}

bool UiaHelper::GetAddressBarUrl(HWND hwnd, std::wstring& urlOut) {
    if (!m_uia || !hwnd) return false;

    IUIAutomationElement* root = nullptr;
    HRESULT hr = m_uia->ElementFromHandle(hwnd, &root);
    if (FAILED(hr) || !root) return false;

    IUIAutomationElement* addr = FindAddressBarElement(root);
    root->Release();

    if (!addr) return false;

    bool ok =
        ReadValueFromElement(addr, urlOut) ||
        ReadTextFromElement(addr, urlOut);

    addr->Release();
    return ok;
}

IUIAutomationElement* UiaHelper::FindAddressBarElement(IUIAutomationElement* root)
{
    if (!m_uia || !root) return nullptr;

    VARIANT vEdit;
    VariantInit(&vEdit);
    vEdit.vt = VT_I4;
    vEdit.lVal = UIA_EditControlTypeId;

    IUIAutomationCondition* condEdit = nullptr;
    m_uia->CreatePropertyCondition(
        UIA_ControlTypePropertyId, vEdit, &condEdit);

    IUIAutomationElementArray* list = nullptr;
    HRESULT hr = root->FindAll(
        TreeScope_Subtree,
        condEdit,
        &list);

    condEdit->Release();

    if (FAILED(hr) || !list) return nullptr;

    int count = 0;
    list->get_Length(&count);

    for (int i = 0; i < count; i++)
    {
        IUIAutomationElement* el = nullptr;
        list->GetElement(i, &el);

        // 🔹 Focus 가능한 Edit만
        BOOL focusable = FALSE;
        el->get_CurrentIsKeyboardFocusable(&focusable);
        if (!focusable)
        {
            el->Release();
            continue;
        }

        // 🔹 Name 검사
        BSTR name = nullptr;
        el->get_CurrentName(&name);

        bool nameMatch = false;
        if (name)
        {
            std::wstring n = name;
            if (n.find(L"Address") != std::wstring::npos ||
                n.find(L"address") != std::wstring::npos ||
                n.find(L"search") != std::wstring::npos ||
                n.find(L"주소") != std::wstring::npos)
            {
                nameMatch = true;
            }
            SysFreeString(name);
        }

        if (!nameMatch)
        {
            el->Release();
            continue;
        }

        // 🔹 값 읽기
        std::wstring value;
        if (ReadValueFromElement(el, value) ||
            ReadTextFromElement(el, value))
        {
            // 느슨한 URL 판별
            if (value.find(L".") != std::wstring::npos &&
                value.find(L" ") == std::wstring::npos &&
                value.length() > 4)
            {
                printf("[UIA] AddressBar FOUND: %ls\n", value.c_str());
                list->Release();
                return el; // caller Release
            }
        }

        el->Release();
    }

    list->Release();
    printf("[UIA] AddressBar element not found\n");
    return nullptr;
}

bool UiaHelper::ReadValueFromElement(IUIAutomationElement* element, std::wstring& out) {
    IUIAutomationValuePattern* vp = nullptr;
    HRESULT hr = element->GetCurrentPatternAs(
        UIA_ValuePatternId,
        __uuidof(IUIAutomationValuePattern),
        (void**)&vp
    );
    if (FAILED(hr) || !vp) return false;

    BSTR bstr = nullptr;
    hr = vp->get_CurrentValue(&bstr);
    vp->Release();

    if (FAILED(hr) || !bstr) return false;

    out.assign(bstr, SysStringLen(bstr));
    SysFreeString(bstr);
    return true;
}

bool UiaHelper::ReadTextFromElement(IUIAutomationElement* element, std::wstring& out) {
    IUIAutomationTextPattern* tp = nullptr;
    HRESULT hr = element->GetCurrentPatternAs(
        UIA_TextPatternId,
        __uuidof(IUIAutomationTextPattern),
        (void**)&tp
    );
    if (FAILED(hr) || !tp) return false;

    IUIAutomationTextRange* range = nullptr;
    tp->get_DocumentRange(&range);
    tp->Release();

    if (!range) return false;

    BSTR bstr = nullptr;
    range->GetText(-1, &bstr);
    range->Release();

    if (!bstr) return false;

    out.assign(bstr, SysStringLen(bstr));
    SysFreeString(bstr);
    return true;
}
