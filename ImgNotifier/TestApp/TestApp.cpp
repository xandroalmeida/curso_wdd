#include <stdio.h>
#include <conio.h>
#include <crtdbg.h>
#include <Windows.h>

#include "..\Common\ImgNotifier.h"


DWORD
GetImageDetail(HANDLE   hDevice)
{
    DWORD               dwError = ERROR_SUCCESS,
                        dwBytes;
    IMG_IMAGE_DETAIL    ImageDetail;

    if (!DeviceIoControl(hDevice,
                         IOCTL_IMG_GET_IMAGE_DETAIL,
                         NULL,
                         0,
                         &ImageDetail,
                         sizeof(ImageDetail),
                         &dwBytes,
                         NULL))
    {
        dwError = GetLastError();
        printf("Erro #%d ao obter detalhes da imagem.\n",
               dwError);
    }
    else
    {
        printf("%s\n",
               ImageDetail.ImageName);
    }
    return dwError;
}



DWORD WINAPI
NotificationThread(PVOID pParam)
{
    HANDLE              hDevice = NULL,
                        hNotificationEvt = NULL,
                        hFinishEvt,
                        hObjects[2];
    DWORD               dwError = ERROR_SUCCESS,
                        dwWait,
                        dwBytes;
    IMG_START_NOTIFYING StartNotifying;

    __try
    {
        hFinishEvt = *(HANDLE*)pParam;

        printf("Abrindo device...\n");

        hDevice = CreateFile("\\\\.\\ImgNotifier",
                             GENERIC_READ | GENERIC_WRITE,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL);

        if (hDevice == INVALID_HANDLE_VALUE)
        {
            dwError = GetLastError();
            printf("Erro #%d ao tentar abrir device.\n",
                   dwError);
            __leave;
        }

        hNotificationEvt = CreateEvent(NULL,
                                       TRUE,
                                       FALSE,
                                       NULL);
        _ASSERT(hNotificationEvt);

        printf("Solicitando inicio de notificação.\n");

        StartNotifying.hEvent = hNotificationEvt;
        if (!DeviceIoControl(hDevice,
                             IOCTL_IMG_START_NOTIFYING,
                             &StartNotifying,
                             sizeof(StartNotifying),
                             NULL,
                             0,
                             &dwBytes,
                             NULL))
        {
            dwError = GetLastError();
            printf("Erro #%d ao iniciar notificação.\n",
                   dwError);
            __leave;
        }

        hObjects[0] = hFinishEvt;
        hObjects[1] = hNotificationEvt;

        do
        {
            dwWait = WaitForMultipleObjects(2,
                                            hObjects,
                                            FALSE,
                                            INFINITE);
            switch(dwWait)
            {
            case WAIT_FAILED:
                dwError = GetLastError();
                printf("Erro #%d ao esperar notificações do device.\n",
                       dwError);
                __leave;

            case WAIT_OBJECT_0 + 1:
                if (GetImageDetail(hDevice) != ERROR_SUCCESS)
                    __leave;
                break;
            }

        } while(dwWait != WAIT_OBJECT_0);

        printf("Solicitando fim de notificação.\n");

        if (!DeviceIoControl(hDevice,
                             IOCTL_IMG_STOP_NOTIFYING,
                             NULL,
                             0,
                             NULL,
                             0,
                             &dwBytes,
                             NULL))
        {
            dwError = GetLastError();
            printf("Erro #%d ao solicitar fim de notificação.\n",
                   dwError);
        }
    }
    __finally
    {
        if (hNotificationEvt)
            CloseHandle(hNotificationEvt);

        if (hDevice && hDevice != INVALID_HANDLE_VALUE)
            CloseHandle(hDevice);
    }

    return dwError;
}



int __cdecl main(int argc, CHAR* argv[])
{
    HANDLE  hThread = NULL,
            hFinishEvt = NULL;
    DWORD   dwError = ERROR_SUCCESS;

    __try
    {
        hFinishEvt = CreateEvent(NULL,
                                 TRUE,
                                 FALSE,
                                 NULL);
        if (!hFinishEvt)
        {
            dwError = GetLastError();
            printf("Erro #%d ao criar evento de finalização.\n",
                   dwError);
            __leave;
        }

        hThread = CreateThread(NULL,
                               0,
                               NotificationThread,
                               &hFinishEvt,
                               0,
                               NULL);
        if (!hThread)
        {
            dwError = GetLastError();
            printf("Erro #%d ao criar thread de notificação.\n",
                   dwError);
            __leave;
        }

        printf("Tecle algo para terminar.\n");
        _getch();

        SetEvent(hFinishEvt);

        WaitForSingleObject(hThread,
                            INFINITE);
    }
    __finally
    {
        if (hThread)
            CloseHandle(hThread);

        if (hFinishEvt)
            CloseHandle(hFinishEvt);
    }

    return dwError;
}

