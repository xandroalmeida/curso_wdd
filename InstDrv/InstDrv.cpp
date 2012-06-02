#include <tchar.h>
#include <stdio.h>
#include <Windows.h>


DWORD RegisterDriver(LPCTSTR    tzDriverName)
{
    SC_HANDLE   hManager = NULL,
                hService = NULL;
    DWORD       dwError = ERROR_SUCCESS;
    TCHAR       tzSystemPath[MAX_PATH];
    TCHAR       tzDriverPath[MAX_PATH];

    __try
    {
        hManager = OpenSCManager(NULL,
                                 NULL,
                                 GENERIC_WRITE | GENERIC_READ);
        if (!hManager)
        {
            dwError = GetLastError();
            __leave;
        }

        GetSystemDirectory(tzSystemPath,
                           ARRAYSIZE(tzSystemPath));

        hService = OpenService(hManager,
                               tzDriverName,
                               GENERIC_READ);
        if (hService)
        {
            dwError = ERROR_FILE_EXISTS;
            __leave;
        }

        _snprintf(tzDriverPath,
                    ARRAYSIZE(tzDriverPath),
                    _T("%s\\drivers\\%s.sys"),
                    tzSystemPath,
                    tzDriverName);

        hService = CreateService(hManager,
                                 tzDriverName,
                                 tzDriverName,
                                 SC_MANAGER_ALL_ACCESS,
                                 SERVICE_KERNEL_DRIVER,
                                 SERVICE_DEMAND_START,
                                 SERVICE_ERROR_NORMAL,
                                 tzDriverPath,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL);
        if (!hService)
        {
            dwError = GetLastError();
            __leave;
        }

        CloseServiceHandle(hService);
        hService = NULL;
    }
    __finally
    {
        if (hManager)
            CloseServiceHandle(hManager);

        if (hService)
            CloseServiceHandle(hService);
    }
    return dwError;
}


DWORD UnregisterDriver(LPCTSTR    tzDriverName)
{
    SC_HANDLE           hManager = NULL,
                        hService = NULL;
    DWORD               dwError = ERROR_SUCCESS;

    __try
    {
        hManager = OpenSCManager(NULL,
                                 NULL,
                                 GENERIC_WRITE | GENERIC_READ);
        if (!hManager)
        {
            dwError = GetLastError();
            __leave;
        }

        hService = OpenService(hManager,
                               tzDriverName,
                               DELETE);
        if (!hService)
        {
            dwError = GetLastError();
            __leave;
        }

        if (!DeleteService(hService))
        {
            dwError = GetLastError();
            __leave;
        }

        CloseServiceHandle(hService);
    }
    __finally
    {
        if (hManager)
            CloseServiceHandle(hManager);

        if (hService)
            CloseServiceHandle(hService);
    }
    return dwError;
}


int __cdecl _tmain(int      argc,
                   TCHAR*   argv[])
{
    BOOL    bError = FALSE;
    DWORD   dwError = ERROR_SUCCESS;

    printf("InstDrv - DriverEntry 2012\n\n");

    if (argc != 3)
    {
        bError = TRUE;
    }
    else
    {
        if (!_tcsicmp(_T("install"),
                      argv[1]))
        {
            dwError = RegisterDriver(argv[2]);

            if (dwError != ERROR_SUCCESS)
                printf("Erro #%d ao tentar instalar driver.\n",
                       dwError);
            else
                printf("Driver instalado com sucesso.\n");
        }
        else if (!_tcsicmp(_T("uninstall"),
                           argv[1]))
        {
            dwError = UnregisterDriver(argv[2]);

            if (dwError != ERROR_SUCCESS)
                printf("Erro #%d ao tentar desinstalar driver.\n",
                       dwError);
            else
                printf("Driver desinstalado com sucesso.\n");
        }
        else
            bError = TRUE;
    }

    if (bError)
        printf("Instalar um driver:\n\tInstDrv install driver_name (sem .sys)\n\n"
               "Desinstalar um driver:\n\tInstDrv uninstall driver_name (sem .sys)\n");

    return dwError;
}
