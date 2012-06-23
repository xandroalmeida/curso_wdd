#include <ntddk.h>
#include "..\Common\ImgNotifier.h"

#define IMG_NTF_TAG    'ftNI'


typedef struct _IMG_EVENT_NODE
{
    LIST_ENTRY          Entry;
    IMG_IMAGE_DETAIL    ImageDetail;

} IMG_EVENT_NODE, *PIMG_EVENT_NODE;

UNICODE_STRING  usSymbolicLink = RTL_CONSTANT_STRING(L"\\DosDevices\\ImgNotifier");
PKEVENT         pkEvent = NULL;
KMUTEX          kEventMtx;
LIST_ENTRY      ListHead;



VOID
OnDriverUnload(IN PDRIVER_OBJECT    pDriverObj)
{
    KdPrint(("ImgNotifier.sys >> OnDriverUnload\n"));

    IoDeleteSymbolicLink(&usSymbolicLink);
    IoDeleteDevice(pDriverObj->DeviceObject);
}



VOID
OnStopNotifying(VOID)
{
    NTSTATUS        nts;
    PLIST_ENTRY     pEntry;
    PIMG_EVENT_NODE pNode;

    KdPrint(("ImgNotifier.sys >> OnStopNotifying\n"));

    nts = KeWaitForMutexObject(&kEventMtx,
                               UserRequest,
                               KernelMode,
                               FALSE,
                               NULL);
    if (!NT_SUCCESS(nts))
        ExRaiseStatus(nts);

    __try
    {
        if (!pkEvent)
            ExRaiseStatus(STATUS_INVALID_DEVICE_STATE);

        ObDereferenceObject(pkEvent);
        pkEvent = NULL;

        while(!IsListEmpty(&ListHead))
        {
            pEntry = RemoveHeadList(&ListHead);

            pNode = CONTAINING_RECORD(pEntry,
                                      IMG_EVENT_NODE,
                                      Entry);
            ExFreePoolWithTag(pNode,
                              IMG_NTF_TAG);
        }
    }
    __finally
    {
        KeReleaseMutex(&kEventMtx,
                       FALSE);
    }
}



NTSTATUS
OnCreate(PDEVICE_OBJECT pDeviceObj,
         PIRP           pIrp)
{
    KdPrint(("ImgNotifier.sys >> OnCreate\n"));

    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(pIrp,IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}



NTSTATUS
OnCleanup(PDEVICE_OBJECT    pDeviceObj,
          PIRP              pIrp)
{
    KdPrint(("ImgNotifier.sys >> OnCleanup\n"));

    __try
    {
        OnStopNotifying();
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp,IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}



NTSTATUS
OnClose(PDEVICE_OBJECT  pDeviceObj,
        PIRP            pIrp)
{
    KdPrint(("ImgNotifier.sys >> OnClose\n"));

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp,IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}



ULONG_PTR
OnStartNotifying(PIMG_START_NOTIFYING  pStartNotifying)
{
    NTSTATUS    nts;

    KdPrint(("ImgNotifier.sys >> OnStartNotifying\n"));

    nts = KeWaitForMutexObject(&kEventMtx,
                               UserRequest,
                               KernelMode,
                               FALSE,
                               NULL);
    if (!NT_SUCCESS(nts))
        ExRaiseStatus(nts);

    __try
    {
        if (pkEvent)
            ExRaiseStatus(STATUS_INVALID_DEVICE_STATE);

        nts =  ObReferenceObjectByHandle(pStartNotifying->hEvent,
                                         EVENT_ALL_ACCESS,
                                         *ExEventObjectType,
                                         KernelMode,
                                         (PVOID*)&pkEvent,
                                         NULL);
        if (!NT_SUCCESS(nts))
            ExRaiseStatus(nts);
    }
    __finally
    {
        KeReleaseMutex(&kEventMtx,
                       FALSE);
    }

    return sizeof(IMG_START_NOTIFYING);
}



ULONG_PTR
OnGetImageDetail(PIMG_IMAGE_DETAIL  pImageDetail)
{
    NTSTATUS        nts;
    PLIST_ENTRY     pEntry;
    PIMG_EVENT_NODE pNode;

    KdPrint(("ImgNotifier.sys >> OnGetImageDetail\n"));

    nts = KeWaitForMutexObject(&kEventMtx,
                               UserRequest,
                               KernelMode,
                               FALSE,
                               NULL);
    if (!NT_SUCCESS(nts))
        ExRaiseStatus(nts);

    __try
    {
        if (IsListEmpty(&ListHead))
            ExRaiseStatus(STATUS_NO_MORE_ENTRIES);

        pEntry = RemoveHeadList(&ListHead);

        pNode = CONTAINING_RECORD(pEntry,
                                  IMG_EVENT_NODE,
                                  Entry);

        RtlCopyMemory(pImageDetail,
                      &pNode->ImageDetail,
                      sizeof(IMG_IMAGE_DETAIL));

        ExFreePoolWithTag(pNode,
                          IMG_NTF_TAG);

        if (IsListEmpty(&ListHead))
            KeResetEvent(pkEvent);
    }
    __finally
    {
        KeReleaseMutex(&kEventMtx,
                       FALSE);
    }

    return sizeof(IMG_IMAGE_DETAIL);
}



NTSTATUS
OnDeviceControl(IN PDEVICE_OBJECT   pDeviceObj,
                IN PIRP             pIrp)
{
    PIO_STACK_LOCATION  pStack;
    PVOID               pBuffer;
    NTSTATUS            nts = STATUS_SUCCESS;
    ULONG_PTR           info = 0;

    KdPrint(("ImgNotifier.sys >> OnDeviceControl\n"));

    __try
    {
        pStack = IoGetCurrentIrpStackLocation(pIrp);
        pBuffer = pIrp->AssociatedIrp.SystemBuffer;

        switch(pStack->Parameters.DeviceIoControl.IoControlCode)
        {
        case IOCTL_IMG_START_NOTIFYING:
            if (pStack->Parameters.DeviceIoControl.InputBufferLength != sizeof(IMG_START_NOTIFYING))
                ExRaiseStatus(STATUS_INVALID_BUFFER_SIZE);

            info = OnStartNotifying((PIMG_START_NOTIFYING)pBuffer);
            break;

        case IOCTL_IMG_GET_IMAGE_DETAIL:
            if (pStack->Parameters.DeviceIoControl.OutputBufferLength != sizeof(IMG_IMAGE_DETAIL))
                ExRaiseStatus(STATUS_INVALID_BUFFER_SIZE);

            info = OnGetImageDetail((PIMG_IMAGE_DETAIL)pBuffer);
            break;

        case IOCTL_IMG_STOP_NOTIFYING:
            OnStopNotifying();
            break;

        default:
            nts = STATUS_INVALID_DEVICE_REQUEST;
            info = 0;
            break;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        nts = GetExceptionCode();
        info = 0;
    }

    pIrp->IoStatus.Status = nts;
    pIrp->IoStatus.Information = info;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return nts;
}



extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT     pDriverObj,
                     PUNICODE_STRING    pusRegistryPath)
{
    PDEVICE_OBJECT  pDeviceObj = NULL;
    UNICODE_STRING  usDeviceName = RTL_CONSTANT_STRING(L"\\Device\\ImgNotifier");
    NTSTATUS        nts;

    KdPrint(("ImgNotifier.sys >> DriverEntry\n"));

    pDriverObj->MajorFunction[IRP_MJ_CREATE] = OnCreate;
    pDriverObj->MajorFunction[IRP_MJ_CLEANUP] = OnCleanup;
    pDriverObj->MajorFunction[IRP_MJ_CLOSE] = OnClose;
    pDriverObj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = OnDeviceControl;

    pDriverObj->DriverUnload = OnDriverUnload;

    KeInitializeMutex(&kEventMtx,
                      0);

    InitializeListHead(&ListHead);

    nts = IoCreateDevice(pDriverObj,
                         0,
                         &usDeviceName,
                         FILE_DEVICE_UNKNOWN,
                         0,
                         TRUE,
                         &pDeviceObj);
    if (!NT_SUCCESS(nts))
        return nts;

    nts = IoCreateSymbolicLink(&usSymbolicLink,
                               &usDeviceName);
    if (!NT_SUCCESS(nts))
        IoDeleteDevice(pDeviceObj);

    return nts;
}
