#include <ntddk.h>

#define STR_LIST_TAG 'LrtS'

UNICODE_STRING      usSymbolicLink = RTL_CONSTANT_STRING(L"\\DosDevices\\StringList");


typedef struct _STRING_ENTRY
{
    UNICODE_STRING  usString;
    LIST_ENTRY      Entry;

} STRING_ENTRY, *PSTRING_ENTRY;


typedef struct _DEVICE_EXTENSION
{
    KSPIN_LOCK  kSpinlock;
    LIST_ENTRY  ListHead;
    ULONG       ulUsers;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;



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
OnDriverUnload(IN PDRIVER_OBJECT  pDriverObj)
{
    KdPrint(("StringList.sys >> OnDriverUnload\n"));

    IoDeleteSymbolicLink(&usSymbolicLink);
    IoDeleteDevice(pDriverObj->DeviceObject);
}


NTSTATUS
OnCreate(PDEVICE_OBJECT pDeviceObj,
         PIRP           pIrp)
{
    PDEVICE_EXTENSION   pDevExtension;
    KIRQL               kIrql;

    KdPrint(("StringList.sys >> OnCreate\n"));

    pDevExtension = (PDEVICE_EXTENSION)pDeviceObj->DeviceExtension;

    KeAcquireSpinLock(&pDevExtension->kSpinlock,
                      &kIrql);

    pDevExtension->ulUsers++;

    KeReleaseSpinLock(&pDevExtension->kSpinlock,
                      kIrql);

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


VOID
ClearList(PLIST_ENTRY   pListHead)
{
    PLIST_ENTRY     pEntry;
    PSTRING_ENTRY   pStringEntry;

    while(!IsListEmpty(pListHead))
    {
        pEntry = RemoveTailList(pListHead);

        pStringEntry = CONTAINING_RECORD(pEntry,
                                         STRING_ENTRY,
                                         Entry);
        ExFreePoolWithTag(pStringEntry,
                          STR_LIST_TAG);
    }
}


NTSTATUS
OnCleanup(PDEVICE_OBJECT    pDeviceObj,
          PIRP              pIrp)
{
    PDEVICE_EXTENSION   pDevExtension;
    PUNICODE_STRING     pusString;
    KIRQL               kIrql;

    KdPrint(("StringList.sys >> OnCleanup\n"));

    pDevExtension = (PDEVICE_EXTENSION)pDeviceObj->DeviceExtension;

    KeAcquireSpinLock(&pDevExtension->kSpinlock,
                      &kIrql);

    if (!--pDevExtension->ulUsers)
        ClearList(&pDevExtension->ListHead);

    KeReleaseSpinLock(&pDevExtension->kSpinlock,
                      kIrql);

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}



NTSTATUS
OnClose(PDEVICE_OBJECT  pDeviceObj,
        PIRP            pIrp)
{
    KdPrint(("StringList.sys >> OnClose\n"));

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}



NTSTATUS
OnRead(IN PDEVICE_OBJECT  pDeviceObj,
       IN PIRP            pIrp)
{
    PDEVICE_EXTENSION   pDevExtension;
    ANSI_STRING         asString;
    PSTRING_ENTRY       pStringEntry = NULL;
    PIO_STACK_LOCATION  pStack;
    ULONG_PTR           info = 0;
    NTSTATUS            nts;
    PLIST_ENTRY         pEntry;

    KdPrint(("StringList.sys >> OnRead\n"));

    __try
    {
        __try
        {
            pStack = IoGetCurrentIrpStackLocation(pIrp);
            pDevExtension = (PDEVICE_EXTENSION)pDeviceObj->DeviceExtension;

            pEntry = ExInterlockedRemoveHeadList(&pDevExtension->ListHead,
                                                 &pDevExtension->kSpinlock);

            if (!pEntry)
                ExRaiseStatus(STATUS_NO_MORE_ENTRIES);

            pStringEntry = CONTAINING_RECORD(pEntry,
                                             STRING_ENTRY,
                                             Entry);

            RtlInitEmptyAnsiString(&asString,
                                   (PCHAR)pIrp->AssociatedIrp.SystemBuffer,
                                   (USHORT)pStack->Parameters.Read.Length);

            nts = RtlUnicodeStringToAnsiString(&asString,
                                               &pStringEntry->usString,
                                               FALSE);
            if (!NT_SUCCESS(nts))
                ExRaiseStatus(nts);

            info = asString.Length + 1;
            nts = STATUS_SUCCESS;
        }
        __finally
        {
            if (pStringEntry)
                ExFreePoolWithTag(pStringEntry,
                                  STR_LIST_TAG);
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



NTSTATUS
OnWrite(IN PDEVICE_OBJECT  pDeviceObj,
        IN PIRP            pIrp)
{
    PIO_STACK_LOCATION  pStack;
    PDEVICE_EXTENSION   pDevExtension;
    ANSI_STRING         asString;
    PSTRING_ENTRY       pStringEntry = NULL;
    ULONG               ulBytes;
    NTSTATUS            nts;
    ULONG_PTR           info = 0;

    KdPrint(("StringList.sys >> OnWrite\n"));

    __try
    {
        __try
        {
            pStack = IoGetCurrentIrpStackLocation(pIrp);
            pDevExtension = (PDEVICE_EXTENSION)pDeviceObj->DeviceExtension;

            if (!IsValidString((PCHAR)pIrp->AssociatedIrp.SystemBuffer,
                               pStack->Parameters.Write.Length))
                ExRaiseStatus(STATUS_INVALID_PARAMETER);

            RtlInitAnsiString(&asString,
                              (PCSZ)pIrp->AssociatedIrp.SystemBuffer);

            ulBytes = sizeof(STRING_ENTRY) +
                      RtlAnsiStringToUnicodeSize(&asString);

            pStringEntry = (PSTRING_ENTRY) ExAllocatePoolWithTag(NonPagedPool,
                                                                 ulBytes,
                                                                 STR_LIST_TAG);
            if (!pStringEntry)
                ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);

            RtlInitEmptyUnicodeString(&pStringEntry->usString,
                                      (PWCHAR)(pStringEntry + 1),
                                      (USHORT)ulBytes - sizeof(STRING_ENTRY));

            nts = RtlAnsiStringToUnicodeString(&pStringEntry->usString,
                                               &asString,
                                               FALSE);
            if (!NT_SUCCESS(nts))
                ExRaiseStatus(nts);

            ExInterlockedInsertTailList(&pDevExtension->ListHead,
                                        &pStringEntry->Entry,
                                        &pDevExtension->kSpinlock);

            pStringEntry = NULL;
            info = pStack->Parameters.Write.Length;
            nts = STATUS_SUCCESS;
        }
        __finally
        {
            if (pStringEntry)
                ExFreePoolWithTag(pStringEntry,
                                  STR_LIST_TAG);
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



extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT  pDriverObj,
            PUNICODE_STRING pusRegistryPath)
{
    NTSTATUS            nts;
    PDEVICE_OBJECT      pDeviceObj;
    PDEVICE_EXTENSION   pDevExtension;
    UNICODE_STRING      usDeviceName = RTL_CONSTANT_STRING(L"\\Device\\StringList");

    KdPrint(("StringList.sys >> DriverEntry\n"));

    pDriverObj->DriverUnload = OnDriverUnload;

    pDriverObj->MajorFunction[IRP_MJ_CREATE] = OnCreate;
    pDriverObj->MajorFunction[IRP_MJ_CLEANUP] = OnCleanup;
    pDriverObj->MajorFunction[IRP_MJ_CLOSE] = OnClose;
    pDriverObj->MajorFunction[IRP_MJ_READ] = OnRead;
    pDriverObj->MajorFunction[IRP_MJ_WRITE] = OnWrite;

    nts = IoCreateDevice(pDriverObj,
                         sizeof(DEVICE_EXTENSION),
                         &usDeviceName,
                         FILE_DEVICE_UNKNOWN,
                         0,
                         FALSE,
                         &pDeviceObj);
    if (!NT_SUCCESS(nts))
        return nts;

    pDeviceObj->Flags |= DO_BUFFERED_IO;

    pDevExtension = (PDEVICE_EXTENSION)pDeviceObj->DeviceExtension;

    KeInitializeSpinLock(&pDevExtension->kSpinlock);
    InitializeListHead(&pDevExtension->ListHead);
    pDevExtension->ulUsers = 0;

    nts = IoCreateSymbolicLink(&usSymbolicLink,
                               &usDeviceName);
    if (!NT_SUCCESS(nts))
        IoDeleteDevice(pDeviceObj);

    return nts;
}
