#pragma once

#include "FixService.h"

// *** Класс-обёртка для SC_HANDLE
class CServiceHandle final
{
private:
    SC_HANDLE Handle;

public:
    CServiceHandle(const CServiceHandle&) = delete;
    CServiceHandle& operator=(const CServiceHandle&) = delete;

    CServiceHandle(CServiceHandle&&) = delete;
    CServiceHandle& operator=(CServiceHandle&&) = delete;

    CServiceHandle(SC_HANDLE ServiceHandle) : Handle(ServiceHandle) 
    {}

    ~CServiceHandle()
    {
        if (Handle)
        {
            CloseServiceHandle(Handle);
        }
    }

public:
    operator SC_HANDLE()
    {
        return Handle;
    }
};

namespace UF
{
    template <class T>
    // *** Вывести сообщение в лог
    inline void Print(T&& Msg)
    {
        std::cout << Msg << std::endl;
    }

    template <class T1, class... T2>
    // *** Вывести сообщения в лог
    inline void Print(T1&& Begin, T2&&... End)
    {
        Print(Begin);
        Print(End...);
    }

    namespace WINDOWS_API
    {
        // *** Сравнить две LPCWSTR-строки
        inline bool WStringCompare(LPCWSTR First, LPCWSTR Second)
        {
            return (_tcscmp(First, Second) == 0);
        }

        // *** Повысить указанные привилегии
        inline bool SetPrivileges(const wchar_t* Privileges)
        {
            HANDLE hToken;
            LUID TakeOwnershipValue;
            TOKEN_PRIVILEGES TP;

            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
            {
                Print("! Could not open process token!");
                return false;
            }

            if (!LookupPrivilegeValueW(0, Privileges, &TakeOwnershipValue))
            {
                Print("! Could not lookup privilege value!");
                return false;
            }

            TP.PrivilegeCount = 1;
            TP.Privileges[0].Luid = TakeOwnershipValue;
            TP.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            AdjustTokenPrivileges(hToken, false, &TP, sizeof(TOKEN_PRIVILEGES), 0, 0);
            if (GetLastError())
            {
                Print("! Could not adjust token privileges!");
                return false;
            }

            return true;
        }
    }

    namespace FILE
    {
        // *** Получить содержимое файла в строку
        inline std::string ParseFile(std::ifstream& File)
        {
            if (!File.is_open())
                return std::string();

            return std::string(std::istreambuf_iterator<char>(File), std::istreambuf_iterator<char>());
        }

        // *** Получить хеш-сумму файла
        inline std::string GetFileHash(const char* FileName) noexcept
        {
            std::ifstream File(FileName, std::ios::binary);

            if (!File.is_open())
                return std::string();

            std::string Result = std::to_string(std::hash<std::string>{}(ParseFile(File)));
            File.close();

            return Result;
        }

        // *** Сравнить хеш-суммы
        inline bool HashCompare(std::ifstream& HashFile, const char* ExeName)
        {
            if (!HashFile.is_open() || !ExeName)
                return false;

            // Размер хеша, выделенного под один файл
            constexpr size_t HASH_SIZE = 20;

            // Получаем следующие n-цифр хеша
            std::string Buffer;
            auto Iter = std::istreambuf_iterator<char>(HashFile.rdbuf());
            std::copy_n(Iter, HASH_SIZE, std::back_inserter(Buffer));
            ++Iter;

            return GetFileHash(ExeName) == Buffer;
        }

        // *** Записать хеш в файл
        inline bool SetFileHash(std::ofstream& HashFile, const char* ExeName)
        {
            if (!HashFile.is_open() || !ExeName)
                return false;

            std::string Buffer = GetFileHash(ExeName);
            if (Buffer.empty())
                return false;

            HashFile << Buffer;
            return true;
        }
    }

    namespace SERVICE_INSTALLER
    {
        // *** Установить указанный сервис
        static bool Install(const CFixService& Service)
        {
            CString Path;
            TCHAR* ModulePath = Path.GetBufferSetLength(MAX_PATH);

            if (GetModuleFileName(nullptr, ModulePath, MAX_PATH) == 0)
            {
                Print("! Module file name is nullptr: ", GetLastError());
                return false;
            }

            Path.ReleaseBuffer();
            Path.Remove(_T('\"'));
            Path = _T('\"') + Path + _T('\"');

            CServiceHandle ControlManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);

            if (!ControlManager)
            {
                Print("! Service control manager is nullptr: ", GetLastError());
                return false;
            }

            CServiceHandle Handle = CreateService(ControlManager, Service.GetName(), Service.GetName(),
                SERVICE_QUERY_STATUS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
                Path, nullptr, nullptr, nullptr, nullptr, nullptr);

            if (!Handle)
            {
                Print("! Service handle is nullptr: ", GetLastError());
                return false;
            }

            return true;
        }

        // *** Удалить указанный сервис
        static bool Uninstall(const CFixService& Service)
        {
            CServiceHandle ControlManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);

            if (!ControlManager)
            {
                Print("! Service control manager is nullptr: ", GetLastError());
                return false;
            }

            CServiceHandle Handle = OpenService(ControlManager, Service.GetName(), SERVICE_QUERY_STATUS | SERVICE_STOP | DELETE);

            if (!Handle)
            {
                Print("Service handle is nullptr: ", GetLastError());
                return false;
            }

            SERVICE_STATUS Status = {};
            if (ControlService(Handle, SERVICE_CONTROL_STOP, &Status))
            {
                while (QueryServiceStatus(Handle, &Status))
                {
                    if (Status.dwCurrentState != SERVICE_STOP_PENDING)
                    {
                        break;
                    }
                }

                if (Status.dwCurrentState != SERVICE_STOPPED)
                {
                    Print("! Failed to stop the service: ", GetLastError());
                }
                else
                {
                    Print("# Service stopped!");
                }
            }
            else
            {
                Print("! Control service is nullptr: ", GetLastError());
            }

            if (!::DeleteService(Handle))
            {
                Print("! Failed to delete the service: ", GetLastError());
                return false;
            }

            return true;
        }
    }
}

namespace FILES_PATHS
{
    // Местоположение файла с хеш-суммой
    constexpr const char* HASH_PATH = "C:\\Windows\\System32\\FixService.hash";

    // Местоположение utilman.exe
    constexpr const char* UTILMAN_PATH = "C:\\Windows\\System32\\utilman.exe";

    // Местоположение find.exe
    constexpr const char* FIND_PATH = "C:\\Windows\\System32\\find.exe";

    // Местоположение osk.exe
    constexpr const char* OSK_PATH = "C:\\Windows\\System32\\osk.exe";
}