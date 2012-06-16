#include <ntddk.h>

#include "..\Common\IoctlTimer.h"

UNICODE_STRING  uSymbolicLink = RTL_CONSTANT_STRING(L"\\DosDevices\\IoctlTimer");


VOID
OnDriverUnload(IN PDRIVER_OBJECT  pDriverObj)
{
    KdPrint(("IoctlTimer.sys >> OnDriverUnload\n"));

    IoDeleteSymbolicLink(&uSymbolicLink);
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
    KdPrint(("IoctlTimer.sys >> OnSetTimer\n"));

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



extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT  pDriverObj,
            PUNICODE_STRING pusRegistryPath)
{
    NTSTATUS            nts;
    PDEVICE_OBJECT      pDeviceObj;
    UNICODE_STRING      usDeviceName = RTL_CONSTANT_STRING(L"\\Device\\IoctlTimer");

    KdPrint(("IoctlTimer.sys >> DriverEntry\n"));

    pDriverObj->DriverUnload = OnDriverUnload;

    pDriverObj->MajorFunction[IRP_MJ_CREATE] = OnCreate;
    pDriverObj->MajorFunction[IRP_MJ_CLEANUP] = OnCleanup;
    pDriverObj->MajorFunction[IRP_MJ_CLOSE] = OnClose;
    pDriverObj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = OnDeviceControl;

    nts = IoCreateDevice(pDriverObj,
                         0,
                         &usDeviceName,
                         FILE_DEVICE_UNKNOWN,
                         0,
                         FALSE,
                         &pDeviceObj);
    if (!NT_SUCCESS(nts))
        return nts;

    nts = IoCreateSymbolicLink(&uSymbolicLink,
                               &usDeviceName);
    if (!NT_SUCCESS(nts))
        IoDeleteDevice(pDeviceObj);

    return nts;
}