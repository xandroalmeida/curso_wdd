#include <ntddk.h>

VOID
OnDriverUnload(PDRIVER_OBJECT   pDriverObj)
{
    UNICODE_STRING  usSymbolicLink = RTL_CONSTANT_STRING(L"\\DosDevices\\OpenClose");

    IoDeleteSymbolicLink(&usSymbolicLink);
    IoDeleteDevice(pDriverObj->DeviceObject);
}


NTSTATUS
OnCreate(PDEVICE_OBJECT pDeviceObj,
         PIRP           pIrp)
{
    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


NTSTATUS
OnClose(PDEVICE_OBJECT  pDeviceObj,
        PIRP            pIrp)
{
    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT  pDriverObj,
            PUNICODE_STRING pusRegistryPath)
{
    NTSTATUS        nts;
    UNICODE_STRING  usDeviceName = RTL_CONSTANT_STRING(L"\\Device\\OpenClose");
    UNICODE_STRING  usSymbolicLink = RTL_CONSTANT_STRING(L"\\DosDevices\\OpenClose");
    PDEVICE_OBJECT  pDeviceObj = NULL;

    pDriverObj->DriverUnload = OnDriverUnload;

    pDriverObj->MajorFunction[IRP_MJ_CREATE] = OnCreate;
    pDriverObj->MajorFunction[IRP_MJ_CLOSE] = OnClose;


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
