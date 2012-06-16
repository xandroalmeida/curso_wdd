#include <ntddk.h>
#include "..\Common\IoctlCopy.h"

UNICODE_STRING  usSymbolicLink = RTL_CONSTANT_STRING(L"\\DosDevices\\IoctlCopy");


VOID
OnDriverUnload(IN PDRIVER_OBJECT    pDriverObj)
{
    KdPrint(("IoctlCopy.sys >> OnDriverUnload\n"));

    IoDeleteSymbolicLink(&usSymbolicLink);
    IoDeleteDevice(pDriverObj->DeviceObject);
}



NTSTATUS
OnCreate(PDEVICE_OBJECT pDeviceObj,
         PIRP           pIrp)
{
    KdPrint(("IoctlCopy.sys >> OnCreate\n"));

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}



NTSTATUS
OnCleanup(PDEVICE_OBJECT    pDeviceObj,
          PIRP              pIrp)
{
    KdPrint(("IoctlCopy.sys >> OnCleanup\n"));

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}



NTSTATUS
OnClose(PDEVICE_OBJECT  pDeviceObj,
        PIRP            pIrp)
{
    KdPrint(("IoctlCopy.sys >> OnClose\n"));

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}



extern "C" NTSTATUS
DriverEntry(IN PDRIVER_OBJECT   pDriverObj,
            IN PUNICODE_STRING  pusRegistryPath)
{
    NTSTATUS        nts;
    PDEVICE_OBJECT  pDeviceObj;
    UNICODE_STRING  usDeviceName = RTL_CONSTANT_STRING(L"\\Device\\IoctlCopy");

    KdPrint(("IoctlCopy.sys >> DriverEntry\n"));

    pDriverObj->MajorFunction[IRP_MJ_CREATE] = OnCreate;
    pDriverObj->MajorFunction[IRP_MJ_CLEANUP] = OnCleanup;
    pDriverObj->MajorFunction[IRP_MJ_CLOSE] = OnClose;

    pDriverObj->DriverUnload = OnDriverUnload;

    nts = IoCreateDevice(pDriverObj,
                         0,
                         &usDeviceName,
                         FILE_DEVICE_UNKNOWN,
                         0,
                         FALSE,
                         &pDeviceObj);

    if (!NT_SUCCESS(nts))
        return nts;

    nts = IoCreateSymbolicLink(&usSymbolicLink,
                               &usDeviceName);
    if (!NT_SUCCESS(nts))
        IoDeleteDevice(pDeviceObj);

    return nts;
}