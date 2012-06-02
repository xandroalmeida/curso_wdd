#include <ntddk.h>

#define READ_WRITE_TAG  'rWdR'


UNICODE_STRING  usSymLinkDirect = RTL_CONSTANT_STRING(L"\\DosDevices\\ReadWrite_Direct");
UNICODE_STRING  usSymLinkBuffered = RTL_CONSTANT_STRING(L"\\DosDevices\\ReadWrite_Buffered");
UNICODE_STRING  usSymLinkNeither = RTL_CONSTANT_STRING(L"\\DosDevices\\ReadWrite_Neither");

PDEVICE_OBJECT  pDevBuffered = NULL;
PDEVICE_OBJECT  pDevDirect = NULL;
PDEVICE_OBJECT  pDevNeither = NULL;
PVOID           pBuffer = NULL;
ULONG           ulBuffer = 0;


VOID
CleanBuffer(VOID)
{
    if (pBuffer)
    {
        ExFreePoolWithTag(pBuffer,
                          READ_WRITE_TAG);
        pBuffer = NULL;
    }

    ulBuffer = 0;
}


VOID
OnDriverUnload(PDRIVER_OBJECT   pDriverObj)
{
    IoDeleteSymbolicLink(&usSymLinkBuffered);
    IoDeleteSymbolicLink(&usSymLinkDirect);
    IoDeleteSymbolicLink(&usSymLinkNeither);

    IoDeleteDevice(pDevBuffered);
    IoDeleteDevice(pDevDirect);
    IoDeleteDevice(pDevNeither);
}


NTSTATUS
OnCreate(PDEVICE_OBJECT pDeviceObj,
         PIRP           pIrp)
{
    KdPrint(("ReadWrite.sys >> OnCreate\n"));

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}


NTSTATUS
OnWrite(PDEVICE_OBJECT  pDeviceObj,
        PIRP            pIrp)
{
    NTSTATUS            nts = STATUS_SUCCESS;
    PIO_STACK_LOCATION  pStack;
    PVOID               pSource = NULL;
    ULONG_PTR           info = 0;

    KdPrint(("ReadWrite.sys >> OnWrite\n"));

    __try
    {
        pStack = IoGetCurrentIrpStackLocation(pIrp);

        CleanBuffer();

        ulBuffer = pStack->Parameters.Write.Length;
        pBuffer = ExAllocatePoolWithTag(PagedPool,
                                        ulBuffer,
                                        READ_WRITE_TAG);
        if (!pBuffer)
            ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);

        if (pDeviceObj == pDevBuffered) {
            pSource = pIrp->AssociatedIrp.SystemBuffer;

		} else if (pDeviceObj == pDevDirect) {
            pSource = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress,
                                                   NormalPagePriority);

		} else if (pDeviceObj == pDevNeither) {
        
            pSource = pIrp->UserBuffer;

            ProbeForRead(pIrp->UserBuffer,
                         ulBuffer,
                         1);
        } else {
            ExRaiseStatus(STATUS_INVALID_PARAMETER);
		}

        RtlCopyMemory(pBuffer,
                      pSource,
                      ulBuffer);

        nts = STATUS_SUCCESS;
        info = ulBuffer;
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
OnRead(PDEVICE_OBJECT  pDeviceObj,
       PIRP            pIrp)
{
    NTSTATUS            nts = STATUS_SUCCESS;
    ULONG_PTR           info = 0;
    PVOID               pTarget = NULL;
    PIO_STACK_LOCATION  pStack;
    ULONG               ulCopy;

    KdPrint(("ReadWrite.sys >> OnRead\n"));

    __try
    {
        pStack = IoGetCurrentIrpStackLocation(pIrp);
        ulCopy = min(pStack->Parameters.Read.Length, ulBuffer);

        if (!pBuffer)
            ExRaiseStatus(STATUS_INVALID_DEVICE_STATE);

        if (pDeviceObj == pDevBuffered)
            pTarget = pIrp->AssociatedIrp.SystemBuffer;

        else if (pDeviceObj == pDevDirect)
            pTarget = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress,
                                                   NormalPagePriority);

        else if (pDeviceObj == pDevNeither)
        {
            ProbeForWrite(pIrp->UserBuffer,
                          ulCopy,
                          1);

            pTarget = pIrp->UserBuffer;
        }

        else
            ExRaiseStatus(STATUS_INVALID_PARAMETER);

        RtlCopyMemory(pTarget,
                      pBuffer,
                      ulCopy);

        nts = STATUS_SUCCESS;
        info = ulCopy;
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
OnCleanup(PDEVICE_OBJECT pDeviceObj,
          PIRP           pIrp)
{
    KdPrint(("ReadWrite.sys >> OnCleanup\n"));

    CleanBuffer();

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}


NTSTATUS
OnClose(PDEVICE_OBJECT  pDeviceObj,
        PIRP            pIrp)
{
    KdPrint(("ReadWrite.sys >> OnClose\n"));

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
    UNICODE_STRING  usDeviceBuffered = RTL_CONSTANT_STRING(L"\\Device\\ReadWrite_Buffered");
    UNICODE_STRING  usDeviceDirect = RTL_CONSTANT_STRING(L"\\Device\\ReadWrite_Direct");
    UNICODE_STRING  usDeviceNeither = RTL_CONSTANT_STRING(L"\\Device\\ReadWrite_Neither");
    BOOLEAN         bSymLinkBuffered = FALSE;
    BOOLEAN         bSymLinkDirect = FALSE;
    BOOLEAN         bSymLinkNeither = FALSE;


    pDriverObj->DriverUnload = OnDriverUnload;

    pDriverObj->MajorFunction[IRP_MJ_CREATE] = OnCreate;
    pDriverObj->MajorFunction[IRP_MJ_READ] = OnRead;
    pDriverObj->MajorFunction[IRP_MJ_WRITE] = OnWrite;
    pDriverObj->MajorFunction[IRP_MJ_CLEANUP] = OnCleanup;
    pDriverObj->MajorFunction[IRP_MJ_CLOSE] = OnClose;

    __try
    {
        nts = IoCreateDevice(pDriverObj,
                             0,
                             &usDeviceBuffered,
                             FILE_DEVICE_UNKNOWN,
                             0,
                             FALSE,
                             &pDevBuffered);
        if (!NT_SUCCESS(nts))
            __leave;

        pDevBuffered->Flags |= DO_BUFFERED_IO;

        nts = IoCreateSymbolicLink(&usSymLinkBuffered,
                                   &usDeviceBuffered);
        if (!NT_SUCCESS(nts))
            __leave;

        bSymLinkBuffered = TRUE;


        nts = IoCreateDevice(pDriverObj,
                             0,
                             &usDeviceDirect,
                             FILE_DEVICE_UNKNOWN,
                             0,
                             FALSE,
                             &pDevDirect);
        if (!NT_SUCCESS(nts))
            __leave;

        pDevDirect->Flags |= DO_DIRECT_IO;

        nts = IoCreateSymbolicLink(&usSymLinkDirect,
                                   &usDeviceDirect);
        if (!NT_SUCCESS(nts))
            __leave;

        bSymLinkDirect = TRUE;


        nts = IoCreateDevice(pDriverObj,
                             0,
                             &usDeviceNeither,
                             FILE_DEVICE_UNKNOWN,
                             0,
                             FALSE,
                             &pDevNeither);
        if (!NT_SUCCESS(nts))
            __leave;

        nts = IoCreateSymbolicLink(&usSymLinkNeither,
                                   &usDeviceNeither);
        if (!NT_SUCCESS(nts))
            __leave;

        bSymLinkNeither = TRUE;
    }
    __finally
    {
        if (!NT_SUCCESS(nts))
        {
            if (bSymLinkBuffered)
                IoDeleteSymbolicLink(&usSymLinkBuffered);
            if (pDevBuffered)
                IoDeleteDevice(pDevBuffered);

            if (bSymLinkDirect)
                IoDeleteSymbolicLink(&usSymLinkDirect);
            if (pDevDirect)
                IoDeleteDevice(pDevDirect);

            if (bSymLinkNeither)
                IoDeleteSymbolicLink(&usSymLinkNeither);
            if (pDevNeither)
                IoDeleteDevice(pDevNeither);
        }
    }

    return nts;
}
