#include <windows.h>
#include <stdio.h>
#include "madCHook.h"
#include "IpcServer.h"
#include "WorkerThread.h"
#include "UrlMonitor.h"

int main() {
    InitializeMadCHook();

    //옵션 리드 시작
    WorkerThread worker;
    worker.Start();

    IpcServer server(&worker);
    server.Start();

    printf("[SYSTEM] Running with Option Reading...\n");

    // URL 모니터 시작
    UrlMonitor urlMonitor(worker.GetDatabase()); // Database 포인터 전달
    urlMonitor.Start();

    printf("[SYSTEM] Running with URL monitoring...\n");

    while (true) Sleep(1000);

    urlMonitor.Stop();
    server.Stop();
    worker.Stop();

    FinalizeMadCHook();
    return 0;
}