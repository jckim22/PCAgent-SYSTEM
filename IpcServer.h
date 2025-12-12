#pragma once
#include <string>
class WorkerThread;

class IpcServer
{
public:
    IpcServer(WorkerThread* worker);
    bool Start();
    void Stop();

private:
    static void __stdcall OnIpcMsg(LPVOID ctx, PVOID pMessage, DWORD dwSize);
    WorkerThread* m_worker = nullptr;
};