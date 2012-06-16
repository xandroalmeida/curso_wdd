#include <ntddk.h>
#include "..\Common\IoctlFilter.h"

#define STR_FLT_TAG 'FtrS'

UNICODE_STRING  usSymbolicLink = RTL_CONSTANT_STRING(L"\\DosDevices\\StringFilter");
PDEVICE_OBJECT  pControlDevObj = NULL;
BOOLEAN         bLogEnabled = FALSE;


PCHAR g_szMajorNames[] =
{
    "IRP_MJ_CREATE",
    "IRP_MJ_CREATE_NAMED_PIPE",
    "IRP_MJ_CLOSE",
    "IRP_MJ_READ",
    "IRP_MJ_WRITE",
    "IRP_MJ_QUERY_INFORMATION",
    "IRP_MJ_SET_INFORMATION",
    "IRP_MJ_QUERY_EA",
    "IRP_MJ_SET_EA",
    "IRP_MJ_FLUSH_BUFFERS",
    "IRP_MJ_QUERY_VOLUME_INFORMATION",
    "IRP_MJ_SET_VOLUME_INFORMATION",
    "IRP_MJ_DIRECTORY_CONTROL",
    "IRP_MJ_FILE_SYSTEM_CONTROL",
    "IRP_MJ_DEVICE_CONTROL",
    "IRP_MJ_INTERNAL_DEVICE_CONTROL",
    "IRP_MJ_SHUTDOWN",
    "IRP_MJ_LOCK_CONTROL",
    "IRP_MJ_CLEANUP",
    "IRP_MJ_CREATE_MAILSLOT",
    "IRP_MJ_QUERY_SECURITY",
    "IRP_MJ_SET_SECURITY",
    "IRP_MJ_POWER",
    "IRP_MJ_SYSTEM_CONTROL",
    "IRP_MJ_DEVICE_CHANGE",
    "IRP_MJ_QUERY_QUOTA",
    "IRP_MJ_SET_QUOTA",
    "IRP_MJ_PNP"
};



VOID
PrintMajorFunction(UCHAR ucMajorFunction,
                   PDEVICE_OBJECT  pDeviceObj)
{
    ASSERT(ucMajorFunction <= IRP_MJ_MAXIMUM_FUNCTION);

    KdPrint(("StringFilter.sys >> %s on %s\n",
            g_szMajorNames[ucMajorFunction],
            pDeviceObj == pControlDevObj ? "Control Device" : "Filter Device"));
}



BOOLEAN
IsValidString(PCHAR pBuffer,
              ULONG ulSize)
{
    BOOLEAN bValid = FALSE;
    ULONG   i = 0;

    while(i < ulSize && !bValid)
    {
        if (!pBuffer[i++])
            bValid = TRUE;
    }

    return bValid;
}



VOID
OnDriverUnload(PDRIVER_OBJECT   pDriverObj)
{
    KdPrint(("StringFilter.sys >> OnDriverUnload\n"));

    IoDeleteSymbolicLink(&usSymbolicLink);
    IoDeleteDevice(pControlDevObj);
}



NTSTATUS
OnCtrlCreate(PDEVICE_OBJECT pDeviceObj,
             PIRP           pIrp)
{
    KdPrint(("StringFilter.sys >> OnCtrlCreate\n"));

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}



NTSTATUS
OnCtrlCleanup(PDEVICE_OBJECT    pDeviceObj,
              PIRP              pIrp)
{
    KdPrint(("StringFilter.sys >> OnCleanup\n"));

    bLogEnabled = FALSE;

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}



NTSTATUS
OnCtrlClose(PDEVICE_OBJECT  pDeviceObj,
            PIRP            pIrp)
{
    KdPrint(("StringFilter.sys >> OnCtrlClose\n"));

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}



NTSTATUS
OnCtrlDeviceControl(PDEVICE_OBJECT  pDeviceObj,
                    PIRP            pIrp)
{
    PIO_STACK_LOCATION  pStack;
    NTSTATUS            nts = STATUS_NOT_IMPLEMENTED;

    KdPrint(("StringFilter.sys >> OnCtrlDeviceControl\n"));

    pStack = IoGetCurrentIrpStackLocation(pIrp);

    switch(pStack->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_FILTER_START:
        bLogEnabled = TRUE;
        nts = STATUS_SUCCESS;
        break;

    case IOCTL_FILTER_STOP:
        bLogEnabled = FALSE;
        nts = STATUS_SUCCESS;
        break;
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
    UNICODE_STRING      usDeviceName = RTL_CONSTANT_STRING(L"\\Device\\StringFilter");

    KdPrint(("StringFilter.sys >> DriverEntry\n"));

    pDriverObj->DriverUnload = OnDriverUnload;

    pDriverObj->MajorFunction[IRP_MJ_CREATE] = OnCtrlCreate;
    pDriverObj->MajorFunction[IRP_MJ_CLEANUP] = OnCtrlCleanup;
    pDriverObj->MajorFunction[IRP_MJ_CLOSE] = OnCtrlClose;
    pDriverObj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = OnCtrlDeviceControl;

    nts = IoCreateDevice(pDriverObj,
                         0,
                         &usDeviceName,
                         FILE_DEVICE_UNKNOWN,
                         0,
                         FALSE,
                         &pControlDevObj);
    if (!NT_SUCCESS(nts))
        return nts;

    nts = IoCreateSymbolicLink(&usSymbolicLink,
                                &usDeviceName);
    if (!NT_SUCCESS(nts))
        IoDeleteDevice(pControlDevObj);

    return nts;
}
