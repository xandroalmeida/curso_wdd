#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <Windows.h>

#include "..\Common\IoctlTimer.h"


int __cdecl _tmain(int argc, _TCHAR* argv[])
{
    HANDLE              hDevice = NULL;
    TIMER_SET_TIMER     TimerSetTimer = { 0 };
    DWORD               dwError, dwBytes;

    __try
    {
        if (argc != 2)
        {
            printf("IoctlTimer - Testa driver de timer\n"
                   "\tModo de uso: IoctlTimer <numero de segundos>\n"
                   "\tEx: IoctlTimer.exe 15\n\n");
            dwError = ERROR_SUCCESS;
            __leave;
        }

        TimerSetTimer.ulSeconds = atoi(argv[1]);
        TimerSetTimer.hEvent = CreateEvent(NULL,
                                           FALSE,
                                           FALSE,
                                           NULL);

        hDevice = CreateFile("\\\\.\\IoctlTimer",
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

        if (!DeviceIoControl(hDevice,
                             IOCTL_TIMER_SET_TIMER,
                             &TimerSetTimer,
                             sizeof(TIMER_SET_TIMER),
                             NULL,
                             0,
                             &dwBytes,
                             NULL))
        {
            dwError = GetLastError();
            printf("Erro #%d ao tentar enviar handle\n",
                   dwError);
            __leave;
        }

        printf("Aguardando evento ser sinalizado...\n");

        dwError = WaitForSingleObject(TimerSetTimer.hEvent,
                                      INFINITE);
        if (dwError == WAIT_FAILED)
        {
            dwError = GetLastError();
            printf("Erro #%d ao esperar evento.\n",
                    dwError);
            __leave;
        }

        printf("Evento sinalizado.\n");
        dwError = ERROR_SUCCESS;
    }
    __finally
    {
        if (TimerSetTimer.hEvent)
            CloseHandle(TimerSetTimer.hEvent);

        if (hDevice && hDevice != INVALID_HANDLE_VALUE)
            CloseHandle(hDevice);
    }

    return dwError;
}

