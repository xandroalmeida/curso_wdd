#include <stdio.h>
#include <conio.h>
#include <Windows.h>


typedef struct _IO_REG
{
    CHAR        szBuffer[100];
    OVERLAPPED  Overlapped;
    _IO_REG*    pNext;

} IO_REG, *PIO_REG;



BOOL
GetString(PCHAR     pBuffer,
          PDWORD    pdwSize)
{
    if (!pdwSize || !*pdwSize)
        return FALSE;

    fflush(stdin);
    *pBuffer = 0;

    scanf_s("%[^\n]",
            pBuffer,
            *pdwSize);

    *pdwSize = strlen(pBuffer) + 1;
    return TRUE;
}



int __cdecl main(int argc,
                 char* argv[])
{
    DWORD       dwBytes,
                dwError;
    HANDLE      hDevice = NULL;
    CHAR        szBuffer[100] = "";
    PIO_REG     pIoReg = NULL,
                pFirst = NULL,
                pLast = NULL;
    int         i = 0;
    OVERLAPPED  OvrRead = { 0 };

    __try
    {
        hDevice = CreateFile("\\\\.\\StringList",
                             GENERIC_READ | GENERIC_WRITE,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             FILE_FLAG_OVERLAPPED,
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
            pIoReg = (PIO_REG) HeapAlloc(GetProcessHeap(),
                                         HEAP_ZERO_MEMORY,
                                         sizeof(IO_REG));
            if (!pIoReg)
            {
                printf("Falha ao tentar alocar OVERLAPPED_REG.\n");
                __leave;
            }

            dwBytes = sizeof(pIoReg->szBuffer);
            GetString(pIoReg->szBuffer,
                      &dwBytes);

            if (!*pIoReg->szBuffer)
            {
                HeapFree(GetProcessHeap(),
                         0,
                         pIoReg);
                break;
            }

            if (!pFirst)
                pFirst = pLast = pIoReg;
            else
                pLast = pLast->pNext = pIoReg;

            pIoReg->Overlapped.hEvent = CreateEvent(NULL,
                                                    TRUE,
                                                    FALSE,
                                                    NULL);
            if (!pIoReg->Overlapped.hEvent)
            {
                printf("Erro #%d ao criar evento.\n",
                       GetLastError());
                __leave;
            }

            if (!WriteFile(hDevice,
                           pIoReg->szBuffer,
                           dwBytes,
                           &dwBytes,
                           &pIoReg->Overlapped))
            {
                dwError = GetLastError();

                if (dwError == ERROR_IO_PENDING)
                    printf("Escrita em andamento...\n");
                else
                {
                    printf("Erro #%d ao escrever no device.\n",
                           dwError);
                    __leave;
                }
            }
            else
                printf("Foram escritos %d bytes com sucesso.\n",
                       dwBytes);

            printf("\nEntre com a próxima string.\n");
        };


        printf("\nAguardando conclusão das escritas.\n");

        pIoReg = pFirst;
        while (pIoReg)
        {
            GetOverlappedResult(hDevice,
                                &pIoReg->Overlapped,
                                &dwBytes,
                                TRUE);

            if (!pIoReg->Overlapped.Internal)
                printf("Escrita da string #%d foi concluída com sucesso (%d bytes enviados).\n",
                       ++i,
                       pIoReg->Overlapped.InternalHigh);
            else
                printf("Escrita da string #%d concluída com erro #%d.\n",
                       ++i,
                       pIoReg->Overlapped.Internal);

            pIoReg = pIoReg->pNext;
        }


        printf("\nIniciando leituras no device.\n");


        OvrRead.hEvent = CreateEvent(NULL,
                                     TRUE,
                                     FALSE,
                                     NULL);
        while(TRUE)
        {
            ZeroMemory(szBuffer,
                       sizeof(szBuffer));

            ResetEvent(OvrRead.hEvent);

            if (!ReadFile(hDevice,
                          szBuffer,
                          sizeof(szBuffer),
                          &dwBytes,
                          &OvrRead))
            {
                dwError = GetLastError();

                if (dwError == ERROR_IO_PENDING)
                {
                    GetOverlappedResult(hDevice,
                                        &OvrRead,
                                        &dwBytes,
                                        TRUE);
                }
                else
                {
                    if (dwError != ERROR_NO_MORE_ITEMS)
                        printf("Erro #%d ao ler do device.\n",
                               dwError);
                    __leave;
                }
            }

            printf("String = %s.\n",
                   szBuffer);
        };

    }
    __finally
    {
        if (OvrRead.hEvent)
            CloseHandle(OvrRead.hEvent);

        while(pFirst)
        {
            pIoReg = pFirst;
            pFirst = pIoReg->pNext;

            if (pIoReg->Overlapped.hEvent)
                CloseHandle(pIoReg->Overlapped.hEvent);

            HeapFree(GetProcessHeap(),
                     0,
                     pIoReg);
        }

        if (hDevice && hDevice != INVALID_HANDLE_VALUE)
            CloseHandle(hDevice);
    }

    return ERROR_SUCCESS;
}
