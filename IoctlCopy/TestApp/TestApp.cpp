#include <conio.h>
#include <stdio.h>
#include <windows.h>

#include "..\Common\IoctlCopy.h"

int __cdecl main(int argc,
                 char* argv[])
{
    char    szInputBuffer[100];
    char    szOutputBuffer[100];
    size_t  stRead = 0;
    HANDLE  hDevice = NULL;
    DWORD   dwError = ERROR_SUCCESS,
            dwBytes,
            i;

    ULONG   ulCtlCodes[] = { IOCTL_COPY_BUFFERED,
                             IOCTL_COPY_DIRECT,
                             IOCTL_COPY_NEITHER };

    __try
    {
        printf("Entre com uma string a ser enviada ao driver: ");
        _cgets_s(szInputBuffer,
                 sizeof(szInputBuffer),
                 &stRead);

        printf("Abrindo o device \"\\\\.\\IoctlCopy\"...\n");
        hDevice = CreateFile("\\\\.\\IoctlCopy",
                             GENERIC_READ | GENERIC_WRITE,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL);

        if (hDevice == INVALID_HANDLE_VALUE)
        {
            dwError = GetLastError();
            printf("Erro #%d ao abrir device...\n",
                   dwError);
            __leave;
        }

        printf("\nEnviando IOCTLs para o device...\n"
               "    Input buffer address:   0x%p\n"
               "    Input buffer size:      %d\n"
               "    Output buffer address:  0x%p\n"
               "    Output buffer size:     %d\n\n",
               szInputBuffer,
               stRead + 1,
               szOutputBuffer,
               sizeof(szOutputBuffer));

        for (i = 0; i < ARRAYSIZE(ulCtlCodes); i++)
        {
            ZeroMemory(szOutputBuffer,
                       sizeof(szOutputBuffer));

            if (!DeviceIoControl(hDevice,
                                 ulCtlCodes[i],
                                 szInputBuffer,
                                 stRead + 1,
                                 szOutputBuffer,
                                 sizeof(szOutputBuffer),
                                 &dwBytes,
                                 NULL))
            {
                dwError = GetLastError();
                printf("Erro #%d ao enviar uma IOCTL...\n",
                        dwError);
                __leave;
            }

            printf("Uma string de %d caracteres foi a obtida do device: %s\n",
                   dwBytes,
                   szOutputBuffer);
        }
    }
    __finally
    {
        if (hDevice && hDevice != INVALID_HANDLE_VALUE)
            CloseHandle(hDevice);
    }

    return dwError;
}
