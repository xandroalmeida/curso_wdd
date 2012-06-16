#include <ntddk.h>

#define STR_STO_TAG 'SrtS'
UNICODE_STRING      usSymLink = RTL_CONSTANT_STRING(L"\\DosDevices\\StoreString");


VOID
OnDriverUnload(IN PDRIVER_OBJECT  pDriverObj)
{
    KdPrint(("StoreString.sys >> OnDriverUnload\n"));

    IoDeleteSymbolicLink(&usSymLink);
    IoDeleteDevice(pDriverObj->DeviceObject);
}



NTSTATUS
OnCreate(PDEVICE_OBJECT pDeviceObj,
         PIRP           pIrp)
{
    PIO_STACK_LOCATION  pStack;
    NTSTATUS            nts;
    PUNICODE_STRING     pusString;

    KdPrint(("StoreString.sys >> OnCreate\n"));

    pStack = IoGetCurrentIrpStackLocation(pIrp);

    pusString = (PUNICODE_STRING) ExAllocatePoolWithTag(NonPagedPool,
                                                        sizeof(UNICODE_STRING),
                                                        STR_STO_TAG);
    if (!pusString)
        nts = STATUS_INSUFFICIENT_RESOURCES;
    else
    {
        RtlZeroMemory(pusString,
                      sizeof(UNICODE_STRING));

        pStack->FileObject->FsContext = pusString;
        nts = STATUS_SUCCESS;
    }

    pIrp->IoStatus.Status = nts;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return nts;
}



NTSTATUS
OnCleanup(PDEVICE_OBJECT    pDeviceObj,
          PIRP              pIrp)
{
    PIO_STACK_LOCATION  pStack;
    PUNICODE_STRING     pusString;

    KdPrint(("StoreString.sys >> OnCleanup\n"));

    pStack = IoGetCurrentIrpStackLocation(pIrp);
    pusString = (PUNICODE_STRING)pStack->FileObject->FsContext;

    if (pusString->Buffer)
        RtlFreeUnicodeString(pusString);

    ExFreePoolWithTag(pusString,
                      STR_STO_TAG);

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}



NTSTATUS
OnClose(PDEVICE_OBJECT  pDeviceObj,
        PIRP            pIrp)
{
    KdPrint(("StoreString.sys >> OnClose\n"));

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}



NTSTATUS
OnRead(IN PDEVICE_OBJECT  pDeviceObj,
       IN PIRP            pIrp)
{
    ANSI_STRING         asString;
    PUNICODE_STRING     pusString;
    PIO_STACK_LOCATION  pStack;
    ULONG_PTR           info = 0;
    NTSTATUS            nts;

    KdPrint(("StoreString.sys >> OnRead\n"));

    __try
    {
        pStack = IoGetCurrentIrpStackLocation(pIrp);

        pusString = (PUNICODE_STRING)pStack->FileObject->FsContext;
        ASSERT(pusString != NULL);

        RtlInitEmptyAnsiString(&asString,
                               (PCHAR)pIrp->AssociatedIrp.SystemBuffer,
                               (USHORT)pStack->Parameters.Read.Length - 1);

        nts = RtlUnicodeStringToAnsiString(&asString,
                                           pusString,
                                           FALSE);
        if (!NT_SUCCESS(nts))
            ExRaiseStatus(nts);

        asString.Buffer[asString.Length] = 0;

        info = asString.Length + 1;
        nts = STATUS_SUCCESS;
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



NTSTATUS
OnWrite(IN PDEVICE_OBJECT  pDeviceObj,
        IN PIRP            pIrp)
{
    PIO_STACK_LOCATION  pStack;
    ANSI_STRING         asString;
    PUNICODE_STRING     pusString;
    NTSTATUS            nts;
    ULONG_PTR           info = 0;

    KdPrint(("StoreString.sys >> OnWrite\n"));

    __try
    {
        pStack = IoGetCurrentIrpStackLocation(pIrp);

        pusString = (PUNICODE_STRING)pStack->FileObject->FsContext;
        ASSERT(pusString != NULL);

        if (pusString->Buffer)
            RtlFreeUnicodeString(pusString);

        RtlInitAnsiString(&asString,
                          (PCSZ)pIrp->AssociatedIrp.SystemBuffer);

        nts = RtlAnsiStringToUnicodeString(pusString,
                                           &asString,
                                           TRUE);
        if (!NT_SUCCESS(nts))
            ExRaiseStatus(nts);

        info = pStack->Parameters.Write.Length;
        nts = STATUS_SUCCESS;
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



extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT  pDriverObj,
            PUNICODE_STRING pusRegistryPath)
{
    NTSTATUS            nts;
    PDEVICE_OBJECT      pDeviceObj;
    UNICODE_STRING      usDeviceName = RTL_CONSTANT_STRING(L"\\Device\\StoreString");

    KdPrint(("StoreString.sys >> DriverEntry\n"));

    pDriverObj->DriverUnload = OnDriverUnload;

    pDriverObj->MajorFunction[IRP_MJ_CREATE] = OnCreate;
    pDriverObj->MajorFunction[IRP_MJ_CLEANUP] = OnCleanup;
    pDriverObj->MajorFunction[IRP_MJ_CLOSE] = OnClose;
    pDriverObj->MajorFunction[IRP_MJ_READ] = OnRead;
    pDriverObj->MajorFunction[IRP_MJ_WRITE] = OnWrite;

    nts = IoCreateDevice(pDriverObj,
                         0,
                         &usDeviceName,
                         FILE_DEVICE_UNKNOWN,
                         0,
                         FALSE,
                         &pDeviceObj);
    if (!NT_SUCCESS(nts))
        return nts;

    pDeviceObj->Flags |= DO_BUFFERED_IO;

    nts = IoCreateSymbolicLink(&usSymLink,
                               &usDeviceName);
    if (!NT_SUCCESS(nts))
        IoDeleteDevice(pDeviceObj);

    return nts;
}


