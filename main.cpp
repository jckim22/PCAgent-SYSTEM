#include <windows.h>
#include <stdio.h>
#include "madCHook.h"
#include "IpcServer.h"
#include "WorkerThread.h"
// #include "Database.h" // main에서 직접 사용하지 않으므로 제거 가능

int main()
{
    InitializeMadCHook();

    // DB 초기화는 WorkerThread 내부에서 수행함
    WorkerThread worker; // 메시지를 처리할 워커 스레드 (NBSP 제거)
    worker.Start();

    IpcServer server(&worker); // IPC 서버에 워커 전달
    server.Start();

    printf("[SYSTEM] Running...\n");

    while (true)
        Sleep(1000);

    server.Stop();
    worker.Stop();

    FinalizeMadCHook();
    return 0;
}