#include <ntddk.h>


typedef struct _DEVICE_EXTENSION
{
    PDEVICE_OBJECT  pNextDeviceObj;

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
OnDriverUnload(PDRIVER_OBJECT   pDriverObj)
{
    PDEVICE_EXTENSION   pDeviceExt;

    KdPrint(("DupString.sys >> OnDriverUnload\n"));

    pDeviceExt = (PDEVICE_EXTENSION)pDriverObj->DeviceObject->DeviceExtension;

    IoDetachDevice(pDeviceExt->pNextDeviceObj);
    IoDeleteDevice(pDriverObj->DeviceObject);

}



NTSTATUS
OnFilterWrite(PDEVICE_OBJECT pDeviceObj,
              PIRP           pIrp)
{
    PIO_STACK_LOCATION  pStack;
    PDEVICE_EXTENSION   pDeviceExt;

    KdPrint(("DupString.sys >> OnFilterWrite\n"));

    pDeviceExt = (PDEVICE_EXTENSION)pDeviceObj->DeviceExtension;

    IoSkipCurrentIrpStackLocation(pIrp);
    return IoCallDriver(pDeviceExt->pNextDeviceObj,
                        pIrp);
}



NTSTATUS
OnDefaultDispatch(PDEVICE_OBJECT    pDeviceObj,
                  PIRP              pIrp)
{
    PDEVICE_EXTENSION   pDeviceExt;
    PIO_STACK_LOCATION  pStack;

    pStack = IoGetCurrentIrpStackLocation(pIrp);

    if (pStack->MajorFunction == IRP_MJ_WRITE)
        return OnFilterWrite(pDeviceObj, pIrp);

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
    PDEVICE_OBJECT      pTargetDevObj = NULL,
                        pFilterDevObj = NULL;
    PFILE_OBJECT        pFileObj = NULL;
    PDEVICE_EXTENSION   pDeviceExt;

    KdPrint(("DupString.sys >> DriverEntry\n"));

    __try
    {
        pDriverObj->DriverUnload = OnDriverUnload;

        for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
            pDriverObj->MajorFunction[i] = OnDefaultDispatch;

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

        if (!NT_SUCCESS(nts) && pFilterDevObj)
            IoDeleteDevice(pFilterDevObj);
    }

    return nts;
}
