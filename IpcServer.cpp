#include <windows.h>
#include "IpcServer.h"
#include "WorkerThread.h"
#include <stdio.h>
#include <string>
#include <string.h>
#include "madCHook.h"

#pragma pack(push,1)
typedef struct _IPC_MSG_HEADER { DWORD nType; DWORD dwSize; } IPC_MSG_HEADER, * PIPC_MSG_HEADER;
#pragma pack(pop)

#define IMT_USER_OPTION_UPDATE 0x8001
#define IMT_URL_EVENT          0x9001

#define IPC_NAME_OPTIONS "UserOptionUpdate"
#define IPC_NAME_URL     "BrowserUrlEvent"

IpcServer::IpcServer(WorkerThread* worker) : m_worker(worker) {}

bool IpcServer::Start() {
    BOOL ok1 = CreateIpcQueue(IPC_NAME_OPTIONS, (PIPC_CALLBACK_ROUTINE)OnIpcMsg, m_worker);
    BOOL ok2 = CreateIpcQueue(IPC_NAME_URL, (PIPC_CALLBACK_ROUTINE)OnUrlMsg, m_worker);
    return ok1 && ok2;
}

void IpcServer::Stop() {
    DestroyIpcQueue(IPC_NAME_OPTIONS);
    DestroyIpcQueue(IPC_NAME_URL);
}

void __stdcall IpcServer::OnIpcMsg(LPVOID ctx, PVOID pMessage, DWORD dwSize) {
    WorkerThread* worker = (WorkerThread*)ctx;
    if (!pMessage || dwSize < sizeof(IPC_MSG_HEADER)) {
        printf("[SYSTEM] Invalid message\n"); return;
    }
    PIPC_MSG_HEADER hdr = (PIPC_MSG_HEADER)pMessage;
    if (hdr->nType != IMT_USER_OPTION_UPDATE) {
        printf("[SYSTEM] Unknown type\n"); return;
    }
    const char* payload = (const char*)pMessage + sizeof(IPC_MSG_HEADER);
    printf("[SYSTEM] Payload: %s\n", payload);
    if (worker) worker->PushMessage(std::string(payload));
    else printf("[SYSTEM] worker ctx is null\n");
}

void __stdcall IpcServer::OnUrlMsg(LPVOID ctx, PVOID pMessage, DWORD dwSize) {
    WorkerThread* worker = (WorkerThread*)ctx;
    if (!pMessage || dwSize < sizeof(IPC_MSG_HEADER)) {
        printf("[SYSTEM] Invalid URL message\n"); return;
    }
    PIPC_MSG_HEADER hdr = (PIPC_MSG_HEADER)pMessage;
    if (hdr->nType != IMT_URL_EVENT) {
        printf("[SYSTEM] Unknown URL type\n"); return;
    }
    const char* payload = (const char*)pMessage + sizeof(IPC_MSG_HEADER);
    printf("[SYSTEM] URL Payload: %s\n", payload);
    if (worker) worker->PushUrlMessage(std::string(payload)); // URL 전용 큐로 전달
    else printf("[SYSTEM] worker ctx is null\n");
}