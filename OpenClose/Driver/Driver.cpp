#include "Driver.h"

static UNICODE_STRING  usSymbolicLink = RTL_CONSTANT_STRING(L"\\DosDevices\\CleanUp");
static UNICODE_STRING  usDeviceName = RTL_CONSTANT_STRING(L"\\Device\\CleanUp");

VOID
OnDriverUnload(PDRIVER_OBJECT   pDriverObj)
{
	if (!strig_list)
		ExFreePoolWithTag(strig_list, 'strl');

    IoDeleteSymbolicLink(&usSymbolicLink);
    IoDeleteDevice(pDriverObj->DeviceObject);
}


NTSTATUS
OnCleanup(PDEVICE_OBJECT pDeviceObj, PIRP pIrp)
{
	DbgPrint("OnCleanup\n");
	PIO_STACK_LOCATION pStack;

	pStack = IoGetCurrentIrpStackLocation(pIrp);
	if (pStack->FileObject->FsContext)
	{
		PUNICODE_STRING us;
		us = (PUNICODE_STRING)pStack->FileObject->FsContext;
		RtlFreeUnicodeString(us);
		ExFreePoolWithTag(us, '1234');

	}

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
	PString_List string_list = pDeviceObj->DeviceExtension;

	Isto deveria estar apenasnoteste

	PString_List us = (PString_List)ExAllocatePoolWithTag(NonPagedPool, sizeof(String_List), '1234');

	us = (PUNICODE_STRING)ExAllocatePoolWithTag(PagedPool, sizeof(UNICODE_STRING), '1234');
	RtlAnsiStringToUnicodeString(us, &as, TRUE);

	pStack->FileObject->FsContext = us;
	
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


	PString_List strig_list;
	strig_list = (PString_List)ExAllocatePoolWithTag(NonPagedPool, sizeof(String_List), 'strl');
	InitializeListHead(&strig_list->entry);
	pDeviceObj->DeviceExtension = strig_list;
	
    return nts;
}
