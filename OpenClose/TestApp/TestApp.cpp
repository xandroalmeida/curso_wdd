#include <stdio.h>
#include <Windows.h>

int __cdecl main(int argc,
                 char* argv[])
{
    HANDLE  hDevice;

    hDevice = CreateFile("\\\\.\\OpenClose",
                         GENERIC_READ,
                         0,
                         NULL,
                         OPEN_EXISTING,
                         0,
                         NULL);

    if (hDevice == INVALID_HANDLE_VALUE)
    {
        printf("Erro #%d ao abrir device.\n",
               GetLastError());
        return 0;
    }

    printf("Device aberto com sucesso!\n");
    CloseHandle(hDevice);

    return 0;
}
