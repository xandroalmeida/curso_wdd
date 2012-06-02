#include <ntddk.h>


static UNICODE_STRING  usSymbolicLink = RTL_CONSTANT_STRING(L"\\DosDevices\\CleanUp");
static UNICODE_STRING  usDeviceName = RTL_CONSTANT_STRING(L"\\Device\\CleanUp");

PVOID buffer = NULL;
ULONG_PTR lenBuffer = 0;

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
	if (buffer)
	{
		ExFreePoolWithTag(buffer, 'rdrw');
	}

    return STATUS_SUCCESS;
}

NTSTATUS
OnWrite(PDEVICE_OBJECT pDeviceObj, PIRP pIrp)
{
	DbgPrint("OnWrite\n");
	
	PIO_STACK_LOCATION sl;

	sl = IoGetCurrentIrpStackLocation(pIrp);
	if (buffer)
		ExFreePoolWithTag(buffer, 'rdrw');

	if ( (buffer = ExAllocatePoolWithTag(PagedPool, sl->Parameters.Write.Length, 'rdrw')) == NULL) {
		DbgPrint("Erro ao alocar memoria\n");
		pIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
		return pIrp->IoStatus.Status;
	}

	RtlZeroMemory(buffer, sl->Parameters.Write.Length);
	RtlCopyMemory(buffer, pIrp->AssociatedIrp.SystemBuffer, sl->Parameters.Write.Length);
	lenBuffer = sl->Parameters.Write.Length;

	pIrp->IoStatus.Status = STATUS_SUCCESS;	
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return pIrp->IoStatus.Status;
}

NTSTATUS
OnRead(PDEVICE_OBJECT pDeviceObj, PIRP pIrp)
{
	DbgPrint("OnRead\n");
	PIO_STACK_LOCATION sl;

	sl = IoGetCurrentIrpStackLocation(pIrp);
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = min(sl->Parameters.Write.Length, lenBuffer);

	min(sl->Parameters.Write.Length, lenBuffer);

	RtlZeroMemory(pIrp->AssociatedIrp.SystemBuffer, sl->Parameters.Read.Length);
	RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer, buffer, min(sl->Parameters.Write.Length, pIrp->IoStatus.Information));

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
	
    return nts;
}
