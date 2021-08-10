#pragma once

#include <iterator>
#include <string>
#include <iostream>
#include <fstream>

#include <Windows.h>
#include <atlstr.h>
#include <WtsApi32.h>

class CFixService final
{
private:
    CString Name;

    SERVICE_STATUS Status;
    SERVICE_STATUS_HANDLE StatusHandle;

    static CFixService* Service;

public:
    CFixService(const CFixService&) = delete;
    CFixService& operator=(const CFixService&) = delete;

    CFixService(CFixService&&) = delete;
    CFixService& operator=(CFixService&&) = delete;

    CFixService();
    ~CFixService() = default;

public:
    bool Run();

private:
    void SetStatus(DWORD dwState, DWORD dwErrCode = NO_ERROR, DWORD dwWait = 0);
    void WriteToEventLog(const CString& msg, WORD type = EVENTLOG_INFORMATION_TYPE);
    void OnStart(DWORD argc, TCHAR* argv[]);

private:
    static void WINAPI SvcMain(DWORD argc, TCHAR* argv[]);
    static DWORD WINAPI ServiceCtrlHandler(DWORD Code, DWORD, void*, void*);

public:
    const CString& GetName() const;

private:
    bool RunInternal(CFixService* svc);
    void Start(DWORD argc, TCHAR* argv[]);
    void Stop();
    void Pause();
    void Continue();
    void Shutdown();
};