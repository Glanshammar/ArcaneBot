#include "DriverFunctions.h"
#include "ArcaneShared.h"

// Control device context
typedef struct _CONTROL_DEVICE_CONTEXT {
    WDFDEVICE Device;
    WDFQUEUE DefaultQueue;
} CONTROL_DEVICE_CONTEXT, * PCONTROL_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CONTROL_DEVICE_CONTEXT, GetControlDeviceContext)

// Forward declarations
NTSTATUS CreateControlDevice(WDFDRIVER Driver);
VOID ControlEvtIoDeviceControl(WDFQUEUE Queue, WDFREQUEST Request, size_t OutputBufferLength,
    size_t InputBufferLength, ULONG IoControlCode);
VOID ControlEvtDeviceContextCleanup(WDFOBJECT DeviceObject);

// Driver Entry
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;

    KdPrint(("ArcaneDriver: DriverEntry\n"));

    // Initialize WDF driver
    WDF_DRIVER_CONFIG_INIT(&config, NULL); // No EvtDeviceAdd, we'll create devices manually
    config.DriverPoolTag = 'narA'; // "Aran" in little-endian
    config.EvtDriverUnload = NULL; // We'll handle cleanup manually

    status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);
    if (!NT_SUCCESS(status)) {
        KdPrint(("ArcaneDriver: WdfDriverCreate failed: 0x%x\n", status));
        return status;
    }

    // Create control device
    status = CreateControlDevice(WdfGetDriver());
    if (!NT_SUCCESS(status)) {
        KdPrint(("ArcaneDriver: Failed to create control device: 0x%x\n", status));
        return status;
    }

    // Create HID keyboard device
    status = VirtualHidKeyboardEvtDeviceAdd(WdfGetDriver(), NULL);
    if (!NT_SUCCESS(status)) {
        KdPrint(("ArcaneDriver: Failed to create HID keyboard device: 0x%x\n", status));
        // Continue anyway, as the control device is more critical
    }

    KdPrint(("ArcaneDriver: Loaded successfully\n"));
    return STATUS_SUCCESS;
}

// Create Control Device
NTSTATUS CreateControlDevice(WDFDRIVER Driver)
{
    NTSTATUS status;
    WDFDEVICE_INIT* deviceInit;
    WDFDEVICE device;
    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    WDF_IO_QUEUE_CONFIG queueConfig;
    UNICODE_STRING deviceName, symbolicLink;
    PCONTROL_DEVICE_CONTEXT deviceContext;

    // Allocate device initialization structure
    deviceInit = WdfControlDeviceInitAllocate(Driver, &SDDL_DEVOBJ_KERNEL_ONLY);
    if (!deviceInit) {
        KdPrint(("ArcaneDriver: WdfControlDeviceInitAllocate failed\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Set device type and characteristics
    WdfDeviceInitSetDeviceType(deviceInit, FILE_DEVICE_UNKNOWN);
    WdfDeviceInitSetExclusive(deviceInit, FALSE);

    // Set up device name
    RtlInitUnicodeString(&deviceName, L"\\Device\\ArcaneDriver");
    status = WdfDeviceInitAssignName(deviceInit, &deviceName);
    if (!NT_SUCCESS(status)) {
        KdPrint(("ArcaneDriver: WdfDeviceInitAssignName failed: 0x%x\n", status));
        WdfDeviceInitFree(deviceInit);
        return status;
    }

    // Set up device context
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, CONTROL_DEVICE_CONTEXT);
    deviceAttributes.EvtCleanupCallback = ControlEvtDeviceContextCleanup;

    status = WdfDeviceCreate(&deviceInit, &deviceAttributes, &device);
    if (!NT_SUCCESS(status)) {
        KdPrint(("ArcaneDriver: WdfDeviceCreate failed: 0x%x\n", status));
        WdfDeviceInitFree(deviceInit);
        return status;
    }

    deviceContext = GetControlDeviceContext(device);
    deviceContext->Device = device;

    // Create symbolic link
    RtlInitUnicodeString(&symbolicLink, L"\\DosDevices\\ArcaneDriver");
    status = WdfDeviceCreateSymbolicLink(device, &symbolicLink);
    if (!NT_SUCCESS(status)) {
        KdPrint(("ArcaneDriver: WdfDeviceCreateSymbolicLink failed: 0x%x\n", status));
        return status;
    }

    // Set up default queue for device control requests
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);
    queueConfig.EvtIoDeviceControl = ControlEvtIoDeviceControl;

    status = WdfIoQueueCreate(device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &deviceContext->DefaultQueue);
    if (!NT_SUCCESS(status)) {
        KdPrint(("ArcaneDriver: WdfIoQueueCreate failed: 0x%x\n", status));
        return status;
    }

    // Control devices must be explicitly started after initialization
    WdfControlFinishInitializing(device);

    KdPrint(("ArcaneDriver: Control device created successfully\n"));
    return STATUS_SUCCESS;
}

// Control Device IOCTL Handler
VOID ControlEvtIoDeviceControl(
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength,
    ULONG IoControlCode)
{
    WDFDEVICE device = WdfIoQueueGetDevice(Queue);
    PDEVICE_CONTEXT deviceContext = GetDeviceContext(device);
    NTSTATUS status = STATUS_SUCCESS;
    size_t bytesReturned = 0;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    KdPrint(("ArcaneDriver: ControlEvtIoDeviceControl - IoControlCode: 0x%x\n", IoControlCode));

    switch (IoControlCode) {
        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            bytesReturned = 0;
            // Fixed the DbgPrint call by removing the extra parentheses
            DbgPrint("ArcaneDriver: Invalid IOCTL: 0x%X\n", IoControlCode);
            break;
        }

    WdfRequestCompleteWithInformation(Request, status, bytesReturned);
}

// Control Device Context Cleanup
VOID ControlEvtDeviceContextCleanup(WDFOBJECT DeviceObject)
{
    UNICODE_STRING symbolicLink;
    NTSTATUS status;

    KdPrint(("ArcaneDriver: ControlEvtDeviceContextCleanup\n"));

    // Delete the symbolic link
    RtlInitUnicodeString(&symbolicLink, L"\\DosDevices\\ArcaneDriver");
    status = IoDeleteSymbolicLink(&symbolicLink);
    if (!NT_SUCCESS(status)) {
        KdPrint(("ArcaneDriver: IoDeleteSymbolicLink failed: 0x%x\n", status));
    }
}

// Driver Unload
VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
    KdPrint(("ArcaneDriver: Unloading\n"));

    // WDF will automatically clean up all devices and resources
    // when the driver object is deleted
}