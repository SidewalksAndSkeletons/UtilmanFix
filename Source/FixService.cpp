#include "FixService.h"
#include "UtilmanFix.h"

CFixService* CFixService::Service = nullptr;

CFixService::CFixService() : Name(_T("UtilmanFix")), StatusHandle(nullptr)
{
    Status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SESSIONCHANGE;
    Status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    Status.dwCurrentState = SERVICE_START_PENDING;
    Status.dwWin32ExitCode = NO_ERROR;
    Status.dwServiceSpecificExitCode = 0;
    Status.dwCheckPoint = 0;
    Status.dwWaitHint = 0;
}

void CFixService::SetStatus(DWORD dwState, DWORD dwErrCode, DWORD dwWait)
{
    Status.dwCurrentState = dwState;
    Status.dwWin32ExitCode = dwErrCode;
    Status.dwWaitHint = dwWait;

    ::SetServiceStatus(StatusHandle, &Status);
}

void CFixService::WriteToEventLog(const CString& msg, WORD type)
{
    HANDLE hSource = RegisterEventSource(nullptr, Name);
    if (hSource)
    {
        const TCHAR* msgData[2] = { Name, msg };
        ReportEvent(hSource, type, 0, 0, nullptr, 2, 0, msgData, nullptr);
        DeregisterEventSource(hSource);
    }
}

void WINAPI CFixService::SvcMain(DWORD argc, TCHAR* argv[])
{
    if (!Service)
    {
        UF::Print("! Service is nullptr!");
        return;
    }

    Service->StatusHandle = ::RegisterServiceCtrlHandlerEx(Service->GetName(), ServiceCtrlHandler, nullptr);
    if (!Service->StatusHandle)
    {
        Service->WriteToEventLog(_T("! Can't set service control handler!"), EVENTLOG_ERROR_TYPE);
        UF::Print("! Can't set service control handler!");
        return;
    }

    Service->Start(argc, argv);
}

DWORD WINAPI CFixService::ServiceCtrlHandler(DWORD Code, DWORD, void*, void*)
{
    switch (Code)
    {
        case SERVICE_CONTROL_STOP:
        {
            Service->Stop();
        } break;

        case SERVICE_CONTROL_PAUSE:
        {
            Service->Pause();
        } break;

        case SERVICE_CONTROL_CONTINUE:
        {
            Service->Continue();
        } break;

        case SERVICE_CONTROL_SHUTDOWN:
        {
            Service->Shutdown();
        } break;

        default:
        {
        } break;
    }

    return 0;
}

bool CFixService::RunInternal(CFixService* svc)
{
    Service = svc;

    TCHAR* Name = const_cast<CString&>(Service->GetName()).GetBuffer();

    SERVICE_TABLE_ENTRY ServiceTable[] =
    {
        { Name, SvcMain },
        { nullptr, nullptr }
    };

    return StartServiceCtrlDispatcher(ServiceTable) == TRUE;
}

void CFixService::Start(DWORD argc, TCHAR* argv[])
{
    SetStatus(SERVICE_START_PENDING);
    OnStart(argc, argv);
    SetStatus(SERVICE_RUNNING);
}

void CFixService::Stop()
{
    SetStatus(SERVICE_STOP_PENDING);
    SetStatus(SERVICE_STOPPED);
}

void CFixService::Pause()
{
    SetStatus(SERVICE_PAUSE_PENDING);
    SetStatus(SERVICE_PAUSED);
}

void CFixService::Continue()
{
    SetStatus(SERVICE_CONTINUE_PENDING);
    SetStatus(SERVICE_RUNNING);
}

void CFixService::Shutdown()
{
    SetStatus(SERVICE_STOPPED);
}

void CFixService::OnStart(DWORD, TCHAR* [])
{
    // Открываем файл с хешем
    std::ifstream File(FILES_PATHS::HASH_PATH, std::ios::binary);
    bool Reading = File.is_open();

    // Режим считывания, сравниваем хеши с уже записанными
    if (Reading)
    {
        // utilman.exe
        bool GoodHash = UF::FILE::HashCompare(File, FILES_PATHS::UTILMAN_PATH);

        // find.exe
        GoodHash &= UF::FILE::HashCompare(File, FILES_PATHS::FIND_PATH);

        // osk.exe
        GoodHash &= UF::FILE::HashCompare(File, FILES_PATHS::OSK_PATH);

        if (!GoodHash)
        {
            // Получаем привилегию на выключение компьютера
            if (!UF::WINDOWS_API::SetPrivileges(SE_SHUTDOWN_NAME))
            {
                UF::Print("! Could not set shutdown privileges!");
                return;
            }

            // Выключаем компьютер (пардон за Legacy API)
            ExitWindowsEx(EWX_FORCE | EWX_SHUTDOWN, 0);
        }
    }
    else
    {
        std::ofstream File(FILES_PATHS::HASH_PATH, std::ios::binary);
        if (!File.is_open())
        {
            UF::Print("! Hash file is corrupted!");
            return;
        }

        // utilman.exe
        bool GoodProcess = UF::FILE::SetFileHash(File, FILES_PATHS::UTILMAN_PATH);

        // find.exe
        GoodProcess &= UF::FILE::SetFileHash(File, FILES_PATHS::FIND_PATH);

        // osk.exe
        GoodProcess &= UF::FILE::SetFileHash(File, FILES_PATHS::OSK_PATH);

        if (!GoodProcess)
        {
            UF::Print("! File's hash is corrupted!");
        }
    }
}

bool CFixService::Run()
{
    return RunInternal(this);
}

const CString& CFixService::GetName() const
{
    return Name;
}