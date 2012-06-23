#include <ntddk.h>
#include <csq.h>


#define CSQ_LIST_TAG    'LrtS'
#define CSQ_LIST_TIME   5

UNICODE_STRING      usSymbolicLink = RTL_CONSTANT_STRING(L"\\DosDevices\\StringList");


typedef struct _STRING_ENTRY
{
    UNICODE_STRING  usString;
    LIST_ENTRY      Entry;

} STRING_ENTRY, *PSTRING_ENTRY;


typedef struct _DEVICE_EXTENSION
{
    KSPIN_LOCK      kSpinlock;
    LIST_ENTRY      ListHead;
    ULONG           ulUsers;
    ULONG           ulSeconds;
    PIO_WORKITEM    pWorkItem;

	IO_CSQ			CancelSafeQueue;
	LIST_ENTRY		IrpListHead;
	KSPIN_LOCK		IrpListLock;
	PIRP			pCurrentItrp;

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
    PDEVICE_EXTENSION   pDevExtension;

    KdPrint(("CsqList.sys >> OnDriverUnload\n"));

    pDevExtension = (PDEVICE_EXTENSION)pDriverObj->DeviceObject->DeviceExtension;

    IoFreeWorkItem(pDevExtension->pWorkItem);

    IoDeleteSymbolicLink(&usSymbolicLink);
    IoDeleteDevice(pDriverObj->DeviceObject);
}



NTSTATUS
OnCreate(PDEVICE_OBJECT pDeviceObj,
         PIRP           pIrp)
{
    PDEVICE_EXTENSION   pDevExtension;
    KIRQL               kIrql;

    KdPrint(("CsqList.sys >> OnCreate\n"));

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
                          CSQ_LIST_TAG);
    }
}



NTSTATUS
OnCleanup(PDEVICE_OBJECT    pDeviceObj,
          PIRP              pIrp)
{
    PDEVICE_EXTENSION   pDevExtension;
    PUNICODE_STRING     pusString;
    KIRQL               kIrql;
	PIO_STACK_LOCATION pStack;
	PIRP pPendingIrp;

    KdPrint(("CsqList.sys >> OnCleanup\n"));

	pStack = IoGetCurrentIrpStackLocation(pIrp);
	pDevExtension = (PDEVICE_EXTENSION)pDeviceObj->DeviceExtension;

	while ((pPendingIrp = IoCsqRemoveNextIrp(&pDevExtension->CancelSafeQueue, pStack->FileObject)))
	{
		pPendingIrp->IoStatus.Status = STATUS_CANCELLED;
		pPendingIrp->IoStatus.Information = 0;
		IoCompleteRequest(pPendingIrp, IO_NO_INCREMENT);
	}

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
    KdPrint(("CsqList.sys >> OnClose\n"));

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

    KdPrint(("CsqList.sys >> OnRead\n"));

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
                                  CSQ_LIST_TAG);
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
    NTSTATUS            nts;

    KdPrint(("CsqList.sys >> OnWrite\n"));

    pStack = IoGetCurrentIrpStackLocation(pIrp);
    pDevExtension = (PDEVICE_EXTENSION)pDeviceObj->DeviceExtension;

    if (!IsValidString((PCHAR)pIrp->AssociatedIrp.SystemBuffer,
                        pStack->Parameters.Write.Length))
    {
        nts = STATUS_INVALID_PARAMETER;

        pIrp->IoStatus.Status = nts;
        pIrp->IoStatus.Information = 0;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    }
    else
    {
		nts = IoCsqInsertIrpEx(&pDevExtension->CancelSafeQueue, pIrp, NULL,NULL);
		if (!NT_SUCCESS(nts)) 
		{
			IoMarkIrpPending(pIrp);

			IoStartPacket(pDeviceObj,
                      pIrp,
                      NULL,
                      NULL);
		}
        nts = STATUS_PENDING;
    }

    return nts;
}



VOID
OnStartIo(PDEVICE_OBJECT    pDeviceObj,
          PIRP              pIrp)
{
    PDEVICE_EXTENSION   pDevExtension;

    pDevExtension = (PDEVICE_EXTENSION)pDeviceObj->DeviceExtension;

    KdPrint(("CsqList.sys >> OnStartIo\n"));

    pDevExtension->ulSeconds = CSQ_LIST_TIME;
    IoStartTimer(pDeviceObj);
}



VOID
OnFinishTimer(PDEVICE_OBJECT    pDeviceObj,
              PVOID             pContext)
{
    ANSI_STRING         asString;
    PDEVICE_EXTENSION   pDevExtension;
    ULONG               ulBytes;
    PIRP                pIrp;
    PSTRING_ENTRY       pStringEntry = NULL;
    NTSTATUS            nts;
    ULONG_PTR           info = 0;
    PIO_STACK_LOCATION  pStack;
    KIRQL               kIrql;

    KdPrint(("CsqList.sys >> OnFinishTimer\n"));

    __try
    {
        __try
        {
            IoStopTimer(pDeviceObj);
			pDevExtension = (PDEVICE_EXTENSION)pDeviceObj->DeviceExtension;

            pIrp = pDeviceObj->CurrentIrp;
			if (pIrp->Cancel)
				ExRaiseStatus(STATUS_CANCELLED);

			pStack = IoGetCurrentIrpStackLocation(pIrp);
            pDevExtension = (PDEVICE_EXTENSION)pDeviceObj->DeviceExtension;

            RtlInitAnsiString(&asString,
                              (PCSZ)pIrp->AssociatedIrp.SystemBuffer);

            ulBytes = sizeof(STRING_ENTRY) +
                      RtlAnsiStringToUnicodeSize(&asString);

            pStringEntry = (PSTRING_ENTRY) ExAllocatePoolWithTag(NonPagedPool,
                                                                 ulBytes,
                                                                 CSQ_LIST_TAG);
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
                                  CSQ_LIST_TAG);
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

	pIrp = IoCsqRemoveNextIrp(&pDevExtension->CancelSafeQueue,NULL);
	
	if (pIrp)
		OnStartIo(pDeviceObj, pIrp);

}



VOID
OnTimer(PDEVICE_OBJECT  pDeviceObj,
        PVOID           pContext)
{
    PDEVICE_EXTENSION   pDevExtension;

    KdPrint(("CsqList.sys >> OnTimer\n"));

    pDevExtension = (PDEVICE_EXTENSION)pDeviceObj->DeviceExtension;

    if (pDevExtension->ulSeconds)
    {
        if (!--pDevExtension->ulSeconds ||
			pDevExtension->pCurrentItrp->Cancel)
        {

			pDevExtension->ulSeconds = 0;
            IoQueueWorkItem(pDevExtension->pWorkItem,
                            OnFinishTimer,
                            DelayedWorkQueue,
                            NULL);
        }
    }
}

VOID
	CsqAcquireLock(PIO_CSQ pCsq, PKIRQL pkIrql)
{
	PDEVICE_EXTENSION pDevExtension;

	pDevExtension = CONTAINING_RECORD(pCsq, DEVICE_EXTENSION, CancelSafeQueue);
	KeAcquireSpinLock(&pDevExtension->IrpListLock, pkIrql);
}

VOID
CsqReleaseLock(PIO_CSQ pCsq, KIRQL kIrql)
{
	PDEVICE_EXTENSION pDevExtension;

	pDevExtension = CONTAINING_RECORD(pCsq, DEVICE_EXTENSION, CancelSafeQueue);
	KeReleaseSpinLock(&pDevExtension->IrpListLock, kIrql);
}

VOID
	CsqCompleteCanceledIrp(PIO_CSQ pCsq, PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_CANCELLED;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
}

PIRP
CsqPeekNextIrp(PIO_CSQ pCsq,
	PIRP pIrp,
	PVOID pContext)
{
	PIO_STACK_LOCATION pStack;
	PDEVICE_EXTENSION pDevExtension;
	PIRP pNextIrp = NULL;
	PLIST_ENTRY pNextEntry;

	pDevExtension = CONTAINING_RECORD(pCsq, DEVICE_EXTENSION, CancelSafeQueue);
	if (pIrp)
		pNextEntry = pIrp->Tail.Overlay.ListEntry.Flink;
	else
		pNextEntry = pDevExtension->IrpListHead.Flink;


	while (pNextEntry != &pDevExtension->IrpListHead)
	{
		pNextIrp = CONTAINING_RECORD(pNextEntry, IRP, Tail.Overlay.ListEntry);

		if (!pContext)
			break;

		pStack = IoGetCurrentIrpStackLocation(pNextIrp);
		if(pStack->FileObject == pContext)
			break;

		pNextIrp = NULL;
		pNextEntry = pNextEntry->Flink;
	}

	if (!pContext)
		pDevExtension->pCurrentItrp = pNextIrp;

	return pNextIrp;
}

VOID
CsqRemoveIrp(PIO_CSQ pCsq,
	PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pCsq);
	RemoveEntryList(&pIrp->Tail.Overlay.ListEntry);
}

NTSTATUS
	CsqInsertIrpEx(PIO_CSQ pCsq,
	PIRP pIrp,
	PVOID pContext)
{
	NTSTATUS nts;
	PDEVICE_EXTENSION pDevExtension;

	pDevExtension = CONTAINING_RECORD(pCsq, DEVICE_EXTENSION, CancelSafeQueue);
	
	if (!pDevExtension->pCurrentItrp)
	{
		pDevExtension->pCurrentItrp = pIrp;
		nts = STATUS_UNSUCCESSFUL;
	}
	else
	{
		InsertTailList(&pDevExtension->IrpListHead, &pIrp->Tail.Overlay.ListEntry);
		nts = STATUS_SUCCESS;
	}

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

    KdPrint(("CsqList.sys >> DriverEntry\n"));

    pDriverObj->DriverUnload = OnDriverUnload;
    pDriverObj->DriverStartIo = OnStartIo;

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

	nts = IoCsqInitializeEx(&pDevExtension->CancelSafeQueue, 
		CsqInsertIrpEx,
		CsqRemoveIrp,
		CsqPeekNextIrp,
		CsqAcquireLock,
		CsqReleaseLock,
		CsqCompleteCanceledIrp);

	if (!NT_SUCCESS(nts))
	{
		IoDeleteDevice(pDeviceObj);
		return nts;
	}
	KeInitializeSpinLock(&pDevExtension->IrpListLock);
	InitializeListHead(&pDevExtension->IrpListHead);
	pDevExtension->pCurrentItrp = NULL;

    nts = IoInitializeTimer(pDeviceObj,
                            OnTimer,
                            NULL);
    if (!NT_SUCCESS(nts))
    {
        IoDeleteDevice(pDeviceObj);
        return nts;
    }

    pDevExtension->pWorkItem = IoAllocateWorkItem(pDeviceObj);
    if (!pDevExtension->pWorkItem)
    {
        IoDeleteDevice(pDeviceObj);
        return nts;
    }

    nts = IoCreateSymbolicLink(&usSymbolicLink,
                               &usDeviceName);
    if (!NT_SUCCESS(nts))
    {
        IoFreeWorkItem(pDevExtension->pWorkItem);
        IoDeleteDevice(pDeviceObj);
    }

    return nts;
}
