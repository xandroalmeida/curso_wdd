#include <ntddk.h>


static UNICODE_STRING  usSymbolicLink = RTL_CONSTANT_STRING(L"\\DosDevices\\CleanUp");
static UNICODE_STRING  usDeviceName = RTL_CONSTANT_STRING(L"\\Device\\CleanUp");

VOID
OnDriverUnload(PDRIVER_OBJECT   pDriverObj)
{
    IoDeleteSymbolicLink(&usSymbolicLink);
    IoDeleteDevice(pDriverObj->DeviceObject);
}


NTSTATUS
OnCleanup(PDEVICE_OBJECT pDeviceObj, PIRP pIrp)
{
	DbgPrint("OnCleanup\n");

    return STATUS_SUCCESS;
}

NTSTATUS
OnCreate(PDEVICE_OBJECT pDeviceObj,
         PIRP           pIrp)
{
	DbgPrint("OnCreate\n");
    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


NTSTATUS
OnClose(PDEVICE_OBJECT  pDeviceObj,
        PIRP            pIrp)
{
	DbgPrint("OnClose\n");
    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT  pDriverObj,
            PUNICODE_STRING pusRegistryPath)
{
	DbgPrint("DriverEntry\n");
    NTSTATUS        nts;
    PDEVICE_OBJECT  pDeviceObj = NULL;

    pDriverObj->DriverUnload = OnDriverUnload;

    pDriverObj->MajorFunction[IRP_MJ_CREATE] = OnCreate;
    pDriverObj->MajorFunction[IRP_MJ_CLOSE] = OnClose;
	pDriverObj->MajorFunction[IRP_MJ_CLEANUP] = OnCleanup;

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
