#include "UtilmanFix.h"

#pragma comment(lib, "Wtsapi32.lib")

// *** Точка входа в программу
int _tmain(int Argc, TCHAR* Argv[])
{
    CFixService Service;

    // Запуск службы
    if (Argc == 1)
    {
        UF::Print("### If you want to install this service, use cmd.exe with command: UtilmanFix.exe install");
        UF::Print("# Try to running...");
        Service.Run();
        return 0;
    }

    if (Argc != 2)
    {
        UF::Print("! Invalide arguments count!");
        return -1;
    }

    // Установка службы
    if (UF::WINDOWS_API::WStringCompare(Argv[1], _T("install")))
    {
        UF::Print("# Installing service...");

        if (!UF::SERVICE_INSTALLER::Install(Service))
        {
            UF::Print("! Could not install the service: ", GetLastError());
            return -1;
        }

        UF::Print("# Service installed!");
        return 0;
    }

    // Удаление службы
    if (UF::WINDOWS_API::WStringCompare(Argv[1], _T("uninstall")))
    {
        UF::Print("# Uninstalling service...");

        if (!UF::SERVICE_INSTALLER::Uninstall(Service))
        {
            UF::Print("! Could not uninstall the service: ", GetLastError());
            return -1;
        }

        UF::Print("# Service uninstalled!");
        return 0;
    }

    UF::Print("! Invalide argument!");
    return -1;
}