#include <ntddk.h>
#include "..\Common\IoctlFilter.h"

#define STR_FLT_TAG 'FtrS'

UNICODE_STRING  usSymbolicLink = RTL_CONSTANT_STRING(L"\\DosDevices\\StringFilter");
PDEVICE_OBJECT  pFilterDevObj = NULL;
PDEVICE_OBJECT  pControlDevObj = NULL;
BOOLEAN         bLogEnabled = FALSE;


typedef struct _DEVICE_EXTENSION
{
    PDEVICE_OBJECT  pNextDeviceObj;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


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
    PDEVICE_EXTENSION   pDeviceExt;

    KdPrint(("StringFilter.sys >> OnDriverUnload\n"));

    pDeviceExt = (PDEVICE_EXTENSION)pFilterDevObj->DeviceExtension;

    IoDetachDevice(pDeviceExt->pNextDeviceObj);
    IoDeleteDevice(pFilterDevObj);

    IoDeleteSymbolicLink(&usSymbolicLink);
    IoDeleteDevice(pControlDevObj);
}


NTSTATUS
OnFilterReadCompletion(PDEVICE_OBJECT   pDeviceObj,
                       PIRP             pIrp,
                       PVOID            pContext)
{
    if (pIrp->PendingReturned)
        IoMarkIrpPending(pIrp);

    if (IsValidString((PCHAR)pIrp->AssociatedIrp.SystemBuffer,
                      pIrp->IoStatus.Information))
    {
        KdPrint(("StringFilter.sys >> LOG READ : %s\n",
                pIrp->AssociatedIrp.SystemBuffer));
    }

    return STATUS_SUCCESS;
}


NTSTATUS
OnFilterRead(PDEVICE_OBJECT pDeviceObj,
             PIRP           pIrp)
{
    PDEVICE_EXTENSION   pDeviceExt;

    KdPrint(("StringFilter.sys >> OnFilterRead\n"));

    pDeviceExt = (PDEVICE_EXTENSION)pDeviceObj->DeviceExtension;

    if (bLogEnabled)
    {
        IoCopyCurrentIrpStackLocationToNext(pIrp);

        IoSetCompletionRoutine(pIrp,
                               OnFilterReadCompletion,
                               NULL,
                               TRUE,
                               FALSE,
                               FALSE);
    }
    else
    {
        IoSkipCurrentIrpStackLocation(pIrp);
    }

    return IoCallDriver(pDeviceExt->pNextDeviceObj,
                        pIrp);
}


NTSTATUS
OnFilterWrite(PDEVICE_OBJECT pDeviceObj,
              PIRP           pIrp)
{
    PIO_STACK_LOCATION  pStack;
    PDEVICE_EXTENSION   pDeviceExt;

    KdPrint(("StringFilter.sys >> OnFilterWrite\n"));

    pStack = IoGetCurrentIrpStackLocation(pIrp);
    pDeviceExt = (PDEVICE_EXTENSION)pDeviceObj->DeviceExtension;

    if (bLogEnabled &&
        IsValidString((PCHAR)pIrp->AssociatedIrp.SystemBuffer,
                      pStack->Parameters.Write.Length))
    {
        KdPrint(("StringFilter.sys >> LOG WRITE : %s\n",
                pIrp->AssociatedIrp.SystemBuffer));
    }

    IoSkipCurrentIrpStackLocation(pIrp);
    return IoCallDriver(pDeviceExt->pNextDeviceObj,
                        pIrp);
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



NTSTATUS
OnDefaultDispatch(PDEVICE_OBJECT    pDeviceObj,
                  PIRP              pIrp)
{
    PDEVICE_EXTENSION   pDeviceExt;
    PIO_STACK_LOCATION  pStack;

    pStack = IoGetCurrentIrpStackLocation(pIrp);

    PrintMajorFunction(pStack->MajorFunction,
                       pDeviceObj);

    if (pDeviceObj == pControlDevObj)
    {
        switch (pStack->MajorFunction)
        {
        case IRP_MJ_CREATE:
            return OnCtrlCreate(pDeviceObj, pIrp);

        case IRP_MJ_CLEANUP:
            return OnCtrlCleanup(pDeviceObj, pIrp);

        case IRP_MJ_CLOSE:
            return OnCtrlClose(pDeviceObj, pIrp);

        case IRP_MJ_DEVICE_CONTROL:
            return OnCtrlDeviceControl(pDeviceObj, pIrp);
        }

        pIrp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        pIrp->IoStatus.Information = 0;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    else
    {
        switch (pStack->MajorFunction)
        {
        case IRP_MJ_READ:
            return OnFilterRead(pDeviceObj, pIrp);

        case IRP_MJ_WRITE:
            return OnFilterWrite(pDeviceObj, pIrp);
        }
    }

    pDeviceExt = (PDEVICE_EXTENSION)pDeviceObj->DeviceExtension;

    IoSkipCurrentIrpStackLocation(pIrp);
    return IoCallDriver(pDeviceExt->pNextDeviceObj,
                        pIrp);
}



extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT  pDriverObj,
            PUNICODE_STRING pusRegistryPath)
{
    NTSTATUS            nts;
    int                 i;
    UNICODE_STRING      usTargetDeviceName = RTL_CONSTANT_STRING(L"\\Device\\StringList");
    UNICODE_STRING      usDeviceName = RTL_CONSTANT_STRING(L"\\Device\\StringFilter");
    PDEVICE_OBJECT      pTargetDevObj = NULL;
    PFILE_OBJECT        pFileObj = NULL;
    PDEVICE_EXTENSION   pDeviceExt;
    BOOLEAN             bSymLink = FALSE;

    KdPrint(("StringFilter.sys >> DriverEntry\n"));

    __try
    {
        pDriverObj->DriverUnload = OnDriverUnload;

        for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
            pDriverObj->MajorFunction[i] = OnDefaultDispatch;

        nts = IoCreateDevice(pDriverObj,
                             0,
                             &usDeviceName,
                             FILE_DEVICE_UNKNOWN,
                             0,
                             FALSE,
                             &pControlDevObj);
        if (!NT_SUCCESS(nts))
            __leave;

        nts = IoCreateSymbolicLink(&usSymbolicLink,
                                   &usDeviceName);
        if (!NT_SUCCESS(nts))
            __leave;

        bSymLink = TRUE;

        nts = IoGetDeviceObjectPointer(&usTargetDeviceName,
                                       FILE_READ_DATA,
                                       &pFileObj,
                                       &pTargetDevObj);
        if (!NT_SUCCESS(nts))
            __leave;

        nts = IoCreateDevice(pDriverObj,
                             sizeof(DEVICE_EXTENSION),
                             NULL,
                             pTargetDevObj->DeviceType,
                             pTargetDevObj->Characteristics,
                             FALSE,
                             &pFilterDevObj);
        if (!NT_SUCCESS(nts))
            __leave;

        pDeviceExt = (PDEVICE_EXTENSION)pFilterDevObj->DeviceExtension;

        pFilterDevObj->Flags |= pTargetDevObj->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO);

        nts = IoAttachDeviceToDeviceStackSafe(pFilterDevObj,
                                              pTargetDevObj,
                                              &pDeviceExt->pNextDeviceObj);
    }
    __finally
    {
        if (pFileObj)
            ObDereferenceObject(pFileObj);

        if (!NT_SUCCESS(nts))
        {
            if (pFilterDevObj)
                IoDeleteDevice(pFilterDevObj);

            if (bSymLink)
                IoDeleteSymbolicLink(&usSymbolicLink);

            if (pControlDevObj)
                IoDeleteDevice(pControlDevObj);
        }
    }

    return nts;
}
