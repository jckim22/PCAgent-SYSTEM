#pragma once
#include <string>

class WorkerThread;

class IpcServer {
public:
    IpcServer(WorkerThread* worker);
    bool Start();
    void Stop();

    static void __stdcall OnIpcMsg(LPVOID ctx, PVOID pMessage, DWORD dwSize);
    static void __stdcall OnUrlMsg(LPVOID ctx, PVOID pMessage, DWORD dwSize); // URL 이벤트용

private:
    WorkerThread* m_worker;
};