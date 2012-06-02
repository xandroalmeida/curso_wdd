#include <stdio.h>
#include <Windows.h>


VOID
ReadWriteDevice(LPCSTR szSymbolicLink)
{
    DWORD   dwBytes;
    HANDLE  hDevice = NULL;
    CHAR    szInput[11] = "0123456789";
    CHAR    szOutput[100] = "";

    __try
    {
        printf("\nObtendo handle para %s.\n",
               szSymbolicLink);

        hDevice = CreateFile(szSymbolicLink,
                             GENERIC_READ | GENERIC_WRITE,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL);

        if (hDevice == INVALID_HANDLE_VALUE)
        {
            printf("Erro #%d ao abrir device %s.\n",
                   GetLastError(),
                   szSymbolicLink);
            __leave;
        }

        printf("Handle para %s aberto com sucesso!\n",
               szSymbolicLink);

        if (!WriteFile(hDevice,
                       szInput,
                       sizeof(szInput),
                       &dwBytes,
                       NULL))
        {
            printf("Erro #%d ao escrever em %s.\n",
                   GetLastError(),
                   szSymbolicLink);
            __leave;
        }

        printf("Foram escritos %d bytes em %s com sucesso.\n",
               dwBytes,
               szSymbolicLink);

        ZeroMemory(szOutput,
                   sizeof(szOutput));

        if (!ReadFile(hDevice,
                      szOutput,
                      sizeof(szOutput),
                      &dwBytes,
                      NULL))
        {
            printf("Erro #%d ao ler de %s.\n",
                   GetLastError(),
                   szSymbolicLink);
            __leave;
        }

        printf("Foram lidos %d bytes de %s com sucesso.\n",
               dwBytes,
               szSymbolicLink);

        printf("Buffer = %s.\n",
               szOutput);
    }
    __finally
    {
        if (hDevice && hDevice != INVALID_HANDLE_VALUE)
            CloseHandle(hDevice);
    }
}


int __cdecl main(int argc,
                 char* argv[])
{
    ReadWriteDevice("\\\\.\\ReadWrite_Buffered");
    ReadWriteDevice("\\\\.\\ReadWrite_Direct");
    ReadWriteDevice("\\\\.\\ReadWrite_Neither");

    return 0;
}
