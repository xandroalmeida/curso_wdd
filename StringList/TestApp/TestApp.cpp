#include <stdio.h>
#include <conio.h>
#include <Windows.h>


int __cdecl main(int argc,
                 char* argv[])
{
    DWORD   dwBytes,
            dwError;
    HANDLE  hDevice = NULL;
    CHAR    szBuffer[100] = "";
    size_t  stBytes;

    __try
    {
        hDevice = CreateFile("\\\\.\\StringList",
                             GENERIC_READ | GENERIC_WRITE,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL);

        if (hDevice == INVALID_HANDLE_VALUE)
        {
            printf("Erro #%d ao abrir device.\n",
                   GetLastError());
            __leave;
        }

        printf("Handle aberto com sucesso!\n\n");

        printf("Entre com uma série de strings.\n"
               "Uma string vazia termina o ciclo de escritas.\n");

        while(TRUE)
        {
            _cgets_s(szBuffer,
                     sizeof(szBuffer),
                     &stBytes);

            if (!*szBuffer)
                break;

            if (!WriteFile(hDevice,
                           szBuffer,
                           stBytes + 1,
                           &dwBytes,
                           NULL))
            {
                printf("Erro #%d ao escrever no device.\n",
                       GetLastError());
                __leave;
            }

            printf("Foram escritos %d bytes com sucesso.\n",
                   dwBytes);
        };


        printf("Iniciando leituras no device.\n");

        while(TRUE)
        {
            ZeroMemory(szBuffer,
                       sizeof(szBuffer));

            if (!ReadFile(hDevice,
                          szBuffer,
                          sizeof(szBuffer),
                          &dwBytes,
                          NULL))
            {
                if ((dwError = GetLastError()) != ERROR_NO_MORE_ITEMS)
                    printf("Erro #%d ao ler do device.\n",
                           dwError);
                __leave;
            }

            printf("String = %s.\n",
                   szBuffer);
        };
    }
    __finally
    {
        if (hDevice && hDevice != INVALID_HANDLE_VALUE)
            CloseHandle(hDevice);
    }

    return ERROR_SUCCESS;
}
