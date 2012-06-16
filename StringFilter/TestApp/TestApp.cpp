#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <Windows.h>

#include "..\Common\IoctlFilter.h"


int __cdecl _tmain(int argc, _TCHAR* argv[])
{
    HANDLE  hDevice = NULL;
    DWORD   dwError, dwBytes;

    __try
    {
        hDevice = CreateFile("\\\\.\\StringFilter",
                             GENERIC_READ | GENERIC_WRITE,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL);

        if (hDevice == INVALID_HANDLE_VALUE)
        {
            dwError = GetLastError();
            printf("Erro #%d ao tentar obter handle para device\n",
                   dwError);
            __leave;
        }

        printf("Handle para device obtido com sucesso.\n");

        if (!DeviceIoControl(hDevice,
                             IOCTL_FILTER_START,
                             NULL,
                             0,
                             NULL,
                             0,
                             &dwBytes,
                             NULL))
        {
            dwError = GetLastError();
            printf("Erro #%d ao tentar habilitar log\n",
                   dwError);
            __leave;
        }

        printf("Log habilitado com sucesso.\n"
                "Tecle algo para terminar.\n");
        _getch();

        if (!DeviceIoControl(hDevice,
                             IOCTL_FILTER_START,
                             NULL,
                             0,
                             NULL,
                             0,
                             &dwBytes,
                             NULL))
        {
            dwError = GetLastError();
            printf("Erro #%d ao tentar habilitar log\n",
                   dwError);
            __leave;
        }

        printf("Log desabilitado com sucesso.\n");
    }
    __finally
    {
        if (hDevice && hDevice != INVALID_HANDLE_VALUE)
            CloseHandle(hDevice);
    }

    return dwError;
}

