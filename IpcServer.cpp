#include <windows.h>
#include "IpcServer.h"
#include "WorkerThread.h"
#include <stdio.h>
#include <string>   // 추가: std::string 사용
#include <string.h>
#include "madCHook.h"

#pragma pack(push,1)
typedef struct _IPC_MSG_HEADER
{
    DWORD nType;
    DWORD dwSize;
} IPC_MSG_HEADER, * PIPC_MSG_HEADER;
#pragma pack(pop)

#define IMT_USER_OPTION_UPDATE  0x8001
#define IPC_NAME "UserOptionUpdate"

IpcServer::IpcServer(WorkerThread* worker)
{
    m_worker = worker;
}

bool IpcServer::Start()
{
    return CreateIpcQueue(IPC_NAME, (PIPC_CALLBACK_ROUTINE)OnIpcMsg, m_worker);
}

void IpcServer::Stop()
{
    DestroyIpcQueue(IPC_NAME);
}

void __stdcall IpcServer::OnIpcMsg(LPVOID ctx, PVOID pMessage, DWORD dwSize)
{
    WorkerThread* worker = (WorkerThread*)ctx;

    if (!pMessage || dwSize < sizeof(IPC_MSG_HEADER))
    {
        printf("[SYSTEM] Invalid message\n");
        return;
    }

    PIPC_MSG_HEADER hdr = (PIPC_MSG_HEADER)pMessage;

    if (hdr->nType != IMT_USER_OPTION_UPDATE)
    {
        printf("[SYSTEM] Unknown type\n");
        return;
    }

    const char* payload = (const char*)pMessage + sizeof(IPC_MSG_HEADER);
    printf("[SYSTEM] Payload: %s\n", payload);

    if (worker)
        worker->PushMessage(std::string(payload));
    else
        printf("[SYSTEM] worker ctx is null\n");
}