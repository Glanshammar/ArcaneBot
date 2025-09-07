#include <ntifs.h>
#include "ArcaneShared.h"
#include "DriverFunctions.h"

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
    case ARCANE_CLICK:
        if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(DWORD32) * 2) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        {
            struct {
                DWORD32 x;
                DWORD32 y;
            }* clickCoords = (decltype(clickCoords))Irp->AssociatedIrp.SystemBuffer;
            DbgPrint("ArcaneBot Driver: Simulating left click at (%d, %d)\n", clickCoords->x, clickCoords->y);
            // Simulate mouse move and left click here
        }
        info = sizeof(DWORD32) * 2;
		break;
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