#include "Driver.h"

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
	PIO_STACK_LOCATION pStack;

	pStack = IoGetCurrentIrpStackLocation(pIrp);
    return STATUS_SUCCESS;
}

NTSTATUS
OnWrite(PDEVICE_OBJECT pDeviceObj, PIRP pIrp)
{
	DbgPrint("OnWrite\n");
	PIO_STACK_LOCATION pStack;

	pStack = IoGetCurrentIrpStackLocation(pIrp);

	ANSI_STRING as;
	as.Buffer = (PCHAR)pIrp->AssociatedIrp.SystemBuffer;
	as.Length = pStack->Parameters.Write.Length;
	as.MaximumLength = pStack->Parameters.Write.Length;

	{
	PString_Entry list = (PString_Entry)pDeviceObj->DeviceExtension;
	PString_Entry entry = (PString_Entry)ExAllocatePoolWithTag(NonPagedPool, sizeof(String_Entry), 'strl');

	PUNICODE_STRING us = (PUNICODE_STRING)ExAllocatePoolWithTag(PagedPool, sizeof(UNICODE_STRING), '1234');
	RtlAnsiStringToUnicodeString(&entry->usString, &as, TRUE);

	InsertTailList(&list->entry, &entry->entry);
	}

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	pIrp->IoStatus.Status = STATUS_SUCCESS;	

    return pIrp->IoStatus.Status;
}

NTSTATUS
OnRead(PDEVICE_OBJECT pDeviceObj, PIRP pIrp)
{
	DbgPrint("OnRead\n");
	PIO_STACK_LOCATION pStack;
	PUNICODE_STRING us;

	pStack = IoGetCurrentIrpStackLocation(pIrp);
	us = (PUNICODE_STRING)pStack->FileObject->FsContext;
	ANSI_STRING as;

	RtlUnicodeStringToAnsiString(&as, us, TRUE);

	RtlZeroMemory(pIrp->AssociatedIrp.SystemBuffer, pStack->Parameters.Read.Length);
	RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer, as.Buffer, min(pStack->Parameters.Write.Length, as.Length));
	pIrp->IoStatus.Information = min(pStack->Parameters.Write.Length, as.Length);

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	pIrp->IoStatus.Status = STATUS_SUCCESS;	
    return pIrp->IoStatus.Status;
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

    pDriverObj->MajorFunction[IRP_MJ_CREATE]	= OnCreate;
    pDriverObj->MajorFunction[IRP_MJ_CLOSE]		= OnClose;
	pDriverObj->MajorFunction[IRP_MJ_CLEANUP]	= OnCleanup;
	pDriverObj->MajorFunction[IRP_MJ_WRITE]		= OnWrite;
	pDriverObj->MajorFunction[IRP_MJ_READ]		= OnRead;

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

    nts = IoCreateSymbolicLink(&usSymbolicLink,
                               &usDeviceName);
    if (!NT_SUCCESS(nts))
        IoDeleteDevice(pDeviceObj);

	PString_Entry strig_entry;
	strig_entry = (PString_Entry)ExAllocatePoolWithTag(NonPagedPool, sizeof(String_Entry), 'strl');
	InitializeListHead(&strig_entry->entry);

	PDevice_Extension de = (PDevice_Extension)ExAllocatePoolWithTag(PagedPool, sizeof(Device_Extension), 'ex00');
	de->listHead = strig_entry->entry;
	KeInitializeSpinLock(&de->slock);
	de->Users = 0;
	pDeviceObj->DeviceExtension = de;
	
    return nts;
}
