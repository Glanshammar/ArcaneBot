#include <ntifs.h>
#include "ArcaneShared.h"
#include "ArcaneFunctions.h"

// Forward declarations
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
VOID DriverUnload(PDRIVER_OBJECT DriverObject);
NTSTATUS DeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS CreateCloseHandler(PDEVICE_OBJECT DeviceObject, PIRP Irp);


NTSTATUS DeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    NTSTATUS status = STATUS_SUCCESS;
    ULONG_PTR info = 0;

    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
    ULONG ioControlCode = stack->Parameters.DeviceIoControl.IoControlCode;

    switch (ioControlCode) {
    case ARCANE_HELLO: {
        PArcaneData inputData = (PArcaneData)Irp->AssociatedIrp.SystemBuffer;

        if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ArcaneData)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        DbgPrint("Message: %S", inputData->Message);
        DbgPrint("Number: %lu", inputData->Number);

        status = STATUS_SUCCESS;
        info = 0;
        break;
    }

    case ARCANE_ASYNC: {
        PArcaneData inputData = (PArcaneData)Irp->AssociatedIrp.SystemBuffer;

        if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ArcaneData)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        // Allocate context for async operation
        PASYNC_CTX asyncCtx = (PASYNC_CTX)ExAllocatePool2(
            POOL_FLAG_NON_PAGED,
            sizeof(ASYNC_CTX),
            'CtxA'
        );

        if (!asyncCtx) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        // Copy the input data to our context
        RtlCopyMemory(&asyncCtx->Data, inputData, sizeof(ArcaneData));
        asyncCtx->Irp = Irp;

        // Create a work item
        asyncCtx->WorkItem = IoAllocateWorkItem(DeviceObject);
        if (!asyncCtx->WorkItem) {
            ExFreePoolWithTag(asyncCtx, 'CtxA');
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        // Mark the IRP as pending
        IoMarkIrpPending(Irp);

        // Queue the work item for execution
        IoQueueWorkItem(
            asyncCtx->WorkItem,
            AsyncWorkRoutine,
            DelayedWorkQueue,
            asyncCtx
        );

        // Return pending status - the IRP will be completed later in the work routine
        return STATUS_PENDING;
    }

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        info = 0;
        DbgPrint("ArcaneBot Driver: Invalid IOCTL: 0x%X\n", ioControlCode);
        break;
    }

    // Complete the IRP for synchronous operations
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = info;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}


NTSTATUS CreateCloseHandler(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(RegistryPath);

    // Set up device and symbolic link
    UNICODE_STRING deviceName, symLink;
    RtlInitUnicodeString(&deviceName, L"\\Device\\ArcaneDriver");
    RtlInitUnicodeString(&symLink, L"\\DosDevices\\ArcaneDriver");

    PDEVICE_OBJECT DeviceObject;
    NTSTATUS status = IoCreateDevice(
        DriverObject,
        0,
        &deviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &DeviceObject
    );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = IoCreateSymbolicLink(&symLink, &deviceName);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(DeviceObject);
        return status;
    }

    // Set up dispatch routines
    DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateCloseHandler;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = CreateCloseHandler;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControl;
    DriverObject->DriverUnload = DriverUnload;

    return STATUS_SUCCESS;
}


VOID DriverUnload(PDRIVER_OBJECT DriverObject) {
    UNICODE_STRING symLink;
    RtlInitUnicodeString(&symLink, L"\\DosDevices\\ArcaneDriver");
    IoDeleteSymbolicLink(&symLink);
    IoDeleteDevice(DriverObject->DeviceObject);
}