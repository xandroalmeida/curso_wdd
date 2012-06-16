#include <stdio.h>
#include <conio.h>
#include <Windows.h>

int __cdecl main(int argc,
                 char* argv[])
{
    DWORD   dwBytes;
    HANDLE  hDevice = NULL;
    CHAR    szBuffer[100] = "";
    size_t  stBytes;

    __try
    {
        hDevice = CreateFile("\\\\.\\StoreString",
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

        printf("Handle aberto com sucesso!\n"
               "Entre com uma string: ");

        _cgets_s(szBuffer,
                 sizeof(szBuffer),
                 &stBytes);

        if (!WriteFile(hDevice,
                       szBuffer,
                       strlen(szBuffer) + 1,
                       &dwBytes,
                       NULL))
        {
            printf("Erro #%d ao escrever no device.\n",
                   GetLastError());
            __leave;
        }

        printf("Foram escritos %d bytes com sucesso.\n"
               "Tecle algo para continuar...\n",
               dwBytes);
        _getch();

        ZeroMemory(szBuffer,
                   sizeof(szBuffer));

        if (!ReadFile(hDevice,
                      szBuffer,
                      sizeof(szBuffer),
                      &dwBytes,
                      NULL))
        {
            printf("Erro #%d ao ler do device.\n",
                   GetLastError());
            __leave;
        }

        printf("Foram lidos %d bytes com sucesso.\n",
               dwBytes);

        printf("String = %s.\n",
               szBuffer);
    }
    __finally
    {
        if (hDevice && hDevice != INVALID_HANDLE_VALUE)
            CloseHandle(hDevice);
    }
    return 0;
}
