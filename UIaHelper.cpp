#include "UiaHelper.h"
#include <vector>
#include <stdio.h>
#include <locale>
#include <codecvt>

UiaHelper::UiaHelper() : m_uia(nullptr), m_initialized(false) {}
UiaHelper::~UiaHelper() { Shutdown(); }

bool UiaHelper::Initialize() {
    if (m_initialized) return true;

    // UIA는 COM 기반이므로, 스레드 안전을 위해 COINIT_MULTITHREADED로 초기화
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        printf("[UIA] CoInitialize failed: 0x%08X\n", hr);
        return false;
    }

    // UIA 객체 생성
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
    // 캐시된 UIA 요소들을 모두 Release
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

// 브라우저 유형을 인자로 받아 URL을 읽어오는 함수
bool UiaHelper::GetAddressBarUrl(HWND hwnd, BrowserType type, std::wstring& urlOut) {
    if (!m_uia || !hwnd || type == BrowserType::Unknown) return false;

    // 1️. 캐시 우선: 이전에 찾은 요소가 있다면 재탐색 없이 사용 시도
    auto it = m_cachedAddr.find(hwnd);
    if (it != m_cachedAddr.end()) {
        if (ReadValueFromElement(it->second, urlOut) ||
            ReadTextFromElement(it->second, urlOut)) {
            return true;
        }
        // 캐시된 요소에서 값을 못 읽으면 (예: 탭 전환으로 요소가 무효화됨) 캐시 제거
        it->second->Release();
        m_cachedAddr.erase(it);
    }

    // 2️. UIA Root: HWND에서 UIA 루트 요소 얻기
    IUIAutomationElement* root = nullptr;
    HRESULT hr = m_uia->ElementFromHandle(hwnd, &root);
    if (FAILED(hr) || !root) return false;

    // 3️. 브라우저 유형에 맞춰 주소 표시줄 요소 탐색
    IUIAutomationElement* addr = FindAddressBarElementByBrowser(root, type);
    root->Release();

    if (!addr) return false;

    // 4️. 캐시: 성공적으로 찾은 요소를 캐시
    addr->AddRef();
    m_cachedAddr[hwnd] = addr;

    // 5️. 값 읽기
    bool ok =
        ReadValueFromElement(addr, urlOut) ||
        ReadTextFromElement(addr, urlOut);

    addr->Release(); // 함수가 반환되기 전에 AddRef했던 요소 Release
    return ok;
}

//브라우저 유형별 주소 표시줄 탐색 로직 (핵심 분기)
IUIAutomationElement* UiaHelper::FindAddressBarElementByBrowser(
    IUIAutomationElement* root, BrowserType type)
{
    // A. 공통 조건: ControlType이 Edit인 요소
    VARIANT vEdit;
    VariantInit(&vEdit);
    vEdit.vt = VT_I4;
    vEdit.lVal = UIA_EditControlTypeId;

    IUIAutomationCondition* condEdit = nullptr;
    m_uia->CreatePropertyCondition(UIA_ControlTypePropertyId, vEdit, &condEdit);

    IUIAutomationElementArray* list = nullptr;
    HRESULT hr = root->FindAll(
        TreeScope_Descendants, // 모든 자손 탐색
        condEdit,
        &list);

    condEdit->Release();
    if (FAILED(hr) || !list) return nullptr;

    int count = 0;
    list->get_Length(&count);

    // 브라우저별 특화된 필터링/탐색 로직
    for (int i = 0; i < count; i++) {
        IUIAutomationElement* el = nullptr;
        list->GetElement(i, &el);

        BSTR name = nullptr;
        el->get_CurrentName(&name); // Name 속성 가져오기

        bool match = false;

        switch (type) {
        case BrowserType::Chrome: // 크롬, 웨일, 엣지 (Chromium) 공통 구조
        case BrowserType::Edge:
        case BrowserType::Whale:
            // Chromium 기반은 Name이 "Address and search bar" / "주소 및 검색 창" 등인 경우가 많음
            if (name) {
                std::wstring n = name;
                if (n.find(L"Address") != std::wstring::npos ||
                    n.find(L"address") != std::wstring::npos ||
                    n.find(L"search") != std::wstring::npos ||
                    n.find(L"주소") != std::wstring::npos) {
                    match = true;
                }
            }
            break;

        case BrowserType::FireFox:
            // Firefox는 UIA ControlType=Edit인 요소의 Name이 빈 경우가 많거나 "Search or enter address" 등
            if (name) {
                std::wstring n = name;
                if (n.find(L"address") != std::wstring::npos ||
                    n.find(L"주소") != std::wstring::npos ||
                    n.find(L"search") != std::wstring::npos) {
                    match = true;
                }
            }
            else {
                // 특정 포커스 가능한 Edit 컨트롤이 주소 표시줄인 경우가 많음 (버전별로 다름)
                BOOL focusable = FALSE;
                el->get_CurrentIsKeyboardFocusable(&focusable);
                if (focusable) {
                    // 추가적인 휴리스틱(예: Automation ID나 Bounding Rect 위치)이 필요할 수 있음
                    match = true;
                }
            }
            break;

        case BrowserType::IE:
            // IE는 일반적으로 ControlType=Edit이고, Name이 "주소" 또는 "Address"인 경우가 많음
            if (name) {
                std::wstring n = name;
                if (n.find(L"Address") != std::wstring::npos ||
                    n.find(L"주소") != std::wstring::npos) {
                    match = true;
                }
            }
            break;

        default:
            // 알 수 없는 브라우저는 기본 로직 적용
            if (name) {
                std::wstring n = name;
                if (n.find(L"Address") != std::wstring::npos ||
                    n.find(L"address") != std::wstring::npos ||
                    n.find(L"search") != std::wstring::npos ||
                    n.find(L"주소") != std::wstring::npos) {
                    match = true;
                }
            }
            break;
        }

        if (name) SysFreeString(name); // Name BSTR 해제

        if (!match) {
            el->Release();
            continue;
        }

        list->Release();
        printf("[UIA] AddressBar FOUND for Browser Type: %d\n", (int)type);
        return el; // 찾은 요소 반환
    }

    list->Release();
    return nullptr;
}


// UIA Value Pattern을 이용해 값 읽기
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

// UIA Text Pattern을 이용해 값 읽기
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