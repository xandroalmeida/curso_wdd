#include <ntddk.h>

#include "..\Common\IoctlTimer.h"

UNICODE_STRING  uSymbolicLink = RTL_CONSTANT_STRING(L"\\DosDevices\\IoctlTimer");


typedef struct _DEVICE_EXTENSION
{
    PKEVENT         pEvent;
    ULONG           ulSeconds;
    BOOLEAN         bInUse;
    KSPIN_LOCK      SpinLock;
    PIO_WORKITEM    pWorkItem;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;



VOID
OnDriverUnload(IN PDRIVER_OBJECT  pDriverObj)
{
    PDEVICE_EXTENSION   pDeviceExt;

    KdPrint(("IoctlTimer.sys >> OnDriverUnload\n"));

    pDeviceExt = (PDEVICE_EXTENSION)pDriverObj->DeviceObject->DeviceExtension;

    IoDeleteSymbolicLink(&uSymbolicLink);

    IoFreeWorkItem(pDeviceExt->pWorkItem);
    IoDeleteDevice(pDriverObj->DeviceObject);
}



NTSTATUS
OnCreate(PDEVICE_OBJECT pDeviceObj,
         PIRP           pIrp)
{
    KdPrint(("IoctlTimer.sys >> OnCreate\n"));

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}



NTSTATUS
OnCleanup(PDEVICE_OBJECT    pDeviceObj,
          PIRP              pIrp)
{
    KdPrint(("IoctlTimer.sys >> OnCleanup\n"));

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}



NTSTATUS
OnClose(PDEVICE_OBJECT  pDeviceObj,
        PIRP            pIrp)
{
    KdPrint(("IoctlTimer.sys >> OnClose\n"));

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}



NTSTATUS
OnSetTimer(IN PDEVICE_OBJECT   pDeviceObj,
           IN PIRP             pIrp)
{
    PDEVICE_EXTENSION   pDeviceExt;
    KIRQL               kIrql;
    NTSTATUS            nts = STATUS_SUCCESS;
    PTIMER_SET_TIMER    pSetTimer;
    PIO_STACK_LOCATION  pStack;

    KdPrint(("IoctlTimer.sys >> OnSetTimer\n"));

    pStack = IoGetCurrentIrpStackLocation(pIrp);
    pDeviceExt = (PDEVICE_EXTENSION)pDeviceObj->DeviceExtension;

    if (pStack->Parameters.DeviceIoControl.InputBufferLength != sizeof(TIMER_SET_TIMER))
        ExRaiseStatus(STATUS_INVALID_PARAMETER);

    KeAcquireSpinLock(&pDeviceExt->SpinLock, &kIrql);

    __try
    {
        if (pDeviceExt->bInUse)
            ExRaiseStatus(STATUS_DEVICE_BUSY);

        pDeviceExt->bInUse = TRUE;
    }
    __finally
    {
        KeReleaseSpinLock(&pDeviceExt->SpinLock, kIrql);
    }

    pSetTimer = (PTIMER_SET_TIMER)pIrp->AssociatedIrp.SystemBuffer;
    nts = ObReferenceObjectByHandle(pSetTimer->hEvent,
                                    EVENT_ALL_ACCESS,
                                    *ExEventObjectType,
                                    KernelMode,
                                    (PVOID*)&pDeviceExt->pEvent,
                                    NULL);
    if (!NT_SUCCESS(nts))
    {
        KeAcquireSpinLock(&pDeviceExt->SpinLock, &kIrql);
        pDeviceExt->bInUse = FALSE;
        KeReleaseSpinLock(&pDeviceExt->SpinLock, kIrql);
        ExRaiseStatus(nts);
    }

    pDeviceExt->ulSeconds = pSetTimer->ulSeconds;

    IoStartTimer(pDeviceObj);

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}



NTSTATUS
OnDeviceControl(IN PDEVICE_OBJECT   pDeviceObj,
                IN PIRP             pIrp)
{
    NTSTATUS            nts = STATUS_INVALID_DEVICE_REQUEST;
    PIO_STACK_LOCATION  pStack;

    KdPrint(("IoctlTimer.sys >> OnDeviceIoControl\n"));

    __try
    {
        pStack = IoGetCurrentIrpStackLocation(pIrp);

        switch(pStack->Parameters.DeviceIoControl.IoControlCode)
        {
        case IOCTL_TIMER_SET_TIMER:
            return OnSetTimer(pDeviceObj, pIrp);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        ASSERT(FALSE);
        nts = GetExceptionCode();
    }

    pIrp->IoStatus.Status = nts;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return nts;
}



VOID
OnFinishTimer(IN PDEVICE_OBJECT pDeviceObj,
              IN PVOID          pContext)
{
    PDEVICE_EXTENSION   pDeviceExt;
    KIRQL               kIrql;

    KdPrint(("IoctlTimer.sys >> OnFinishTimer\n"));

    pDeviceExt = (PDEVICE_EXTENSION)pDeviceObj->DeviceExtension;

    KeSetEvent(pDeviceExt->pEvent,
               IO_NO_INCREMENT,
               FALSE);

    IoStopTimer(pDeviceObj);

    ObDereferenceObject(pDeviceExt->pEvent);
    pDeviceExt->pEvent = NULL;

    KeAcquireSpinLock(&pDeviceExt->SpinLock, &kIrql);
    pDeviceExt->bInUse = FALSE;
    KeReleaseSpinLock(&pDeviceExt->SpinLock, kIrql);
}


VOID
OnIoTimer(IN PDEVICE_OBJECT pDeviceObj,
          IN PVOID          pContext)
{
    PDEVICE_EXTENSION   pDeviceExt;

    KdPrint(("IoctlTimer.sys >> OnIoTimer\n"));

    pDeviceExt = (PDEVICE_EXTENSION)pDeviceObj->DeviceExtension;

    KeAcquireSpinLockAtDpcLevel(&pDeviceExt->SpinLock);

    if (pDeviceExt->ulSeconds)
    {
        if (!--pDeviceExt->ulSeconds)
        {
            IoQueueWorkItem((PIO_WORKITEM)pContext,
                            OnFinishTimer,
                            DelayedWorkQueue,
                            NULL);
        }
    }

    KeReleaseSpinLockFromDpcLevel(&pDeviceExt->SpinLock);
}


extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT  pDriverObj,
            PUNICODE_STRING pusRegistryPath)
{
    NTSTATUS            nts;
    PDEVICE_OBJECT      pDeviceObj;
    UNICODE_STRING      usDeviceName = RTL_CONSTANT_STRING(L"\\Device\\IoctlTimer");
    PDEVICE_EXTENSION   pDeviceExt;

    KdPrint(("IoctlTimer.sys >> DriverEntry\n"));

    pDriverObj->DriverUnload = OnDriverUnload;

    pDriverObj->MajorFunction[IRP_MJ_CREATE] = OnCreate;
    pDriverObj->MajorFunction[IRP_MJ_CLEANUP] = OnCleanup;
    pDriverObj->MajorFunction[IRP_MJ_CLOSE] = OnClose;

    pDriverObj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = OnDeviceControl;

    nts = IoCreateDevice(pDriverObj,
                         sizeof(DEVICE_EXTENSION),
                         &usDeviceName,
                         FILE_DEVICE_UNKNOWN,
                         0,
                         FALSE,
                         &pDeviceObj);
    if (!NT_SUCCESS(nts))
        return nts;

    pDeviceExt = (PDEVICE_EXTENSION)pDeviceObj->DeviceExtension;

    pDeviceExt->pEvent = NULL;
    pDeviceExt->ulSeconds = 0;
    pDeviceExt->bInUse = FALSE;
    KeInitializeSpinLock(&pDeviceExt->SpinLock);

    nts = IoCreateSymbolicLink(&uSymbolicLink,
                               &usDeviceName);
    if (!NT_SUCCESS(nts))
    {
        IoDeleteDevice(pDeviceObj);
        return nts;
    }

    if (!(pDeviceExt->pWorkItem = IoAllocateWorkItem(pDeviceObj)))
    {
        IoDeleteDevice(pDeviceObj);
        IoDeleteSymbolicLink(&uSymbolicLink);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    nts = IoInitializeTimer(pDeviceObj,
                            OnIoTimer,
                            pDeviceExt->pWorkItem);
    if (!NT_SUCCESS(nts))
    {
        IoFreeWorkItem(pDeviceExt->pWorkItem);
        IoDeleteDevice(pDeviceObj);
        IoDeleteSymbolicLink(&uSymbolicLink);
        return nts;
    }

    return STATUS_SUCCESS;
}