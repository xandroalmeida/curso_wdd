#include <ntddk.h>
#include "..\Common\IoctlCopy.h"

UNICODE_STRING  usSymbolicLink = RTL_CONSTANT_STRING(L"\\DosDevices\\IoctlCopy");


VOID
OnDriverUnload(IN PDRIVER_OBJECT    pDriverObj)
{
    KdPrint(("IoctlCopy.sys >> OnDriverUnload\n"));

    IoDeleteSymbolicLink(&usSymbolicLink);
    IoDeleteDevice(pDriverObj->DeviceObject);
}



NTSTATUS
OnCopyBuffered(PDEVICE_OBJECT   pDeviceObj,
               PIRP             pIrp)
{
    PIO_STACK_LOCATION  pStack;

    pStack = IoGetCurrentIrpStackLocation(pIrp);

    KdPrint(("========== OnCopyBuffered ===========\n"
             "Input buffer address: 0x%p\n"
             "Input buffer size:    %d\n"
             "Output buffer addres: 0x%p\n"
             "Output buffer size:   %d\n\n",
             pIrp->AssociatedIrp.SystemBuffer,
             pStack->Parameters.DeviceIoControl.InputBufferLength,
             pIrp->AssociatedIrp.SystemBuffer,
             pStack->Parameters.DeviceIoControl.OutputBufferLength));

    if (pStack->Parameters.DeviceIoControl.OutputBufferLength <
        pStack->Parameters.DeviceIoControl.InputBufferLength)
        ExRaiseStatus(STATUS_BUFFER_TOO_SMALL);

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = pStack->Parameters.DeviceIoControl.InputBufferLength;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}



NTSTATUS
OnCopyDirect(PDEVICE_OBJECT pDeviceObj,
             PIRP           pIrp)
{
    PIO_STACK_LOCATION  pStack;
    PVOID               pOutputBuffer;

    pStack = IoGetCurrentIrpStackLocation(pIrp);

    pOutputBuffer = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress,
                                                 LowPagePriority);

    if (!pOutputBuffer)
        ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);

    KdPrint(("=========== OnCopyDirect ============\n"
             "Input buffer address: 0x%p\n"
             "Input buffer size:    %d\n"
             "Output buffer addres: 0x%p\n"
             "Output buffer size:   %d\n\n",
             pIrp->AssociatedIrp.SystemBuffer,
             pStack->Parameters.DeviceIoControl.InputBufferLength,
             pOutputBuffer,
             pStack->Parameters.DeviceIoControl.OutputBufferLength));

    if (pStack->Parameters.DeviceIoControl.OutputBufferLength <
        pStack->Parameters.DeviceIoControl.InputBufferLength)
        ExRaiseStatus(STATUS_BUFFER_TOO_SMALL);

    RtlCopyMemory(pOutputBuffer,
                  pIrp->AssociatedIrp.SystemBuffer,
                  pStack->Parameters.DeviceIoControl.InputBufferLength);

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = pStack->Parameters.DeviceIoControl.InputBufferLength;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}



NTSTATUS
OnCopyNeither(IN PDEVICE_OBJECT pDeviceObj,
              IN PIRP           pIrp)
{
    PIO_STACK_LOCATION  pStack;

    pStack = IoGetCurrentIrpStackLocation(pIrp);

    KdPrint(("=========== OnCopyNeither ===========\n"
             "Input buffer address: 0x%p\n"
             "Input buffer size:    %d\n"
             "Output buffer addres: 0x%p\n"
             "Output buffer size:   %d\n\n",
             pStack->Parameters.DeviceIoControl.Type3InputBuffer,
             pStack->Parameters.DeviceIoControl.InputBufferLength,
             pIrp->UserBuffer,
             pStack->Parameters.DeviceIoControl.OutputBufferLength));

    if (pIrp->Tail.Overlay.Thread != PsGetCurrentThread())
        ExRaiseStatus(STATUS_INVALID_ADDRESS);

    ProbeForRead(pStack->Parameters.DeviceIoControl.Type3InputBuffer,
                 pStack->Parameters.DeviceIoControl.InputBufferLength,
                 1);

    ProbeForWrite(pIrp->UserBuffer,
                  pStack->Parameters.DeviceIoControl.OutputBufferLength,
                  1);

    if (pStack->Parameters.DeviceIoControl.OutputBufferLength <
        pStack->Parameters.DeviceIoControl.InputBufferLength)
        ExRaiseStatus(STATUS_BUFFER_TOO_SMALL);

    RtlCopyMemory(pIrp->UserBuffer,
                  pStack->Parameters.DeviceIoControl.Type3InputBuffer,
                  pStack->Parameters.DeviceIoControl.InputBufferLength);

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = pStack->Parameters.DeviceIoControl.InputBufferLength;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}



NTSTATUS
OnCreate(PDEVICE_OBJECT pDeviceObj,
         PIRP           pIrp)
{
    KdPrint(("IoctlCopy.sys >> OnCreate\n"));

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}



NTSTATUS
OnCleanup(PDEVICE_OBJECT    pDeviceObj,
          PIRP              pIrp)
{
    KdPrint(("IoctlCopy.sys >> OnCleanup\n"));

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}



NTSTATUS
OnClose(PDEVICE_OBJECT  pDeviceObj,
        PIRP            pIrp)
{
    KdPrint(("IoctlCopy.sys >> OnClose\n"));

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}



NTSTATUS
OnDeviceControl(IN PDEVICE_OBJECT   pDeviceObj,
                IN PIRP             pIrp)
{
    PIO_STACK_LOCATION  pStack;
    NTSTATUS            nts = STATUS_NOT_IMPLEMENTED;

    KdPrint(("IoctlCopy.sys >> OnDeviceControl\n"));

    __try
    {
        pStack = IoGetCurrentIrpStackLocation(pIrp);

        switch(pStack->Parameters.DeviceIoControl.IoControlCode)
        {
        case IOCTL_COPY_BUFFERED:
            return OnCopyBuffered(pDeviceObj,
                                  pIrp);

        case IOCTL_COPY_DIRECT:
            return OnCopyDirect(pDeviceObj,
                                pIrp);

        case IOCTL_COPY_NEITHER:
            return OnCopyNeither(pDeviceObj,
                                 pIrp);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        nts = GetExceptionCode();
    }

    pIrp->IoStatus.Status = nts;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return nts;
}



extern "C" NTSTATUS
DriverEntry(IN PDRIVER_OBJECT   pDriverObj,
            IN PUNICODE_STRING  pusRegistryPath)
{
    NTSTATUS        nts;
    PDEVICE_OBJECT  pDeviceObj;
    UNICODE_STRING  usDeviceName = RTL_CONSTANT_STRING(L"\\Device\\IoctlCopy");

    KdPrint(("IoctlCopy.sys >> DriverEntry\n"));

    pDriverObj->MajorFunction[IRP_MJ_CREATE] = OnCreate;
    pDriverObj->MajorFunction[IRP_MJ_CLEANUP] = OnCleanup;
    pDriverObj->MajorFunction[IRP_MJ_CLOSE] = OnClose;
    pDriverObj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = OnDeviceControl;

    pDriverObj->DriverUnload = OnDriverUnload;

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