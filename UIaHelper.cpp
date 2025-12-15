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
    for (auto& it : m_cachedAddr) {
        if (it.second) it.second->Release();
    }
    m_cachedAddr.clear();

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

    // 1️⃣ 캐시 우선
    auto it = m_cachedAddr.find(hwnd);
    if (it != m_cachedAddr.end()) {
        if (ReadValueFromElement(it->second, urlOut) ||
            ReadTextFromElement(it->second, urlOut)) {
            return true;
        }
    }

    // 2️⃣ UIA Root
    IUIAutomationElement* root = nullptr;
    HRESULT hr = m_uia->ElementFromHandle(hwnd, &root);
    if (FAILED(hr) || !root) return false;

    IUIAutomationElement* addr = FindAddressBarElement(root);
    root->Release();

    if (!addr) return false;

    // 캐시
    addr->AddRef();
    m_cachedAddr[hwnd] = addr;

    bool ok =
        ReadValueFromElement(addr, urlOut) ||
        ReadTextFromElement(addr, urlOut);

    addr->Release();
    return ok;
}

IUIAutomationElement* UiaHelper::FindAddressBarElement(IUIAutomationElement* root)
{
    VARIANT vEdit;
    VariantInit(&vEdit);
    vEdit.vt = VT_I4;
    vEdit.lVal = UIA_EditControlTypeId;

    IUIAutomationCondition* condEdit = nullptr;
    m_uia->CreatePropertyCondition(
        UIA_ControlTypePropertyId, vEdit, &condEdit);

    IUIAutomationElementArray* list = nullptr;
    HRESULT hr = root->FindAll(
        TreeScope_Descendants,
        condEdit,
        &list);

    condEdit->Release();
    if (FAILED(hr) || !list) return nullptr;

    int count = 0;
    list->get_Length(&count);

    for (int i = 0; i < count; i++) {
        IUIAutomationElement* el = nullptr;
        list->GetElement(i, &el);

        BOOL focusable = FALSE;
        el->get_CurrentIsKeyboardFocusable(&focusable);
        if (!focusable) {
            el->Release();
            continue;
        }

        BSTR name = nullptr;
        el->get_CurrentName(&name);

        bool match = false;
        if (name) {
            std::wstring n = name;
            if (n.find(L"Address") != std::wstring::npos ||
                n.find(L"address") != std::wstring::npos ||
                n.find(L"search") != std::wstring::npos ||
                n.find(L"주소") != std::wstring::npos) {
                match = true;
            }
            SysFreeString(name);
        }

        if (!match) {
            el->Release();
            continue;
        }

        list->Release();
        printf("[UIA] AddressBar FOUND\n");
        return el; // caller Release
    }

    list->Release();
    return nullptr;
}

bool UiaHelper::ReadValueFromElement(IUIAutomationElement* element, std::wstring& out) {
    IUIAutomationValuePattern* vp = nullptr;
    if (FAILED(element->GetCurrentPatternAs(
        UIA_ValuePatternId,
        __uuidof(IUIAutomationValuePattern),
        (void**)&vp))) return false;

    BSTR bstr = nullptr;
    vp->get_CurrentValue(&bstr);
    vp->Release();

    if (!bstr) return false;
    out.assign(bstr, SysStringLen(bstr));
    SysFreeString(bstr);
    return true;
}

bool UiaHelper::ReadTextFromElement(IUIAutomationElement* element, std::wstring& out) {
    IUIAutomationTextPattern* tp = nullptr;
    if (FAILED(element->GetCurrentPatternAs(
        UIA_TextPatternId,
        __uuidof(IUIAutomationTextPattern),
        (void**)&tp))) return false;

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
