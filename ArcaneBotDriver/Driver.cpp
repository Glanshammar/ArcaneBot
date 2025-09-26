#include "DriverFunctions.h"
#include "ArcaneShared.h"

// Control device context
typedef struct _CONTROL_DEVICE_CONTEXT {
    WDFDEVICE Device;
    WDFQUEUE DefaultQueue;
} CONTROL_DEVICE_CONTEXT, * PCONTROL_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CONTROL_DEVICE_CONTEXT, GetControlDeviceContext)

// Driver Entry
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;

    KdPrint(("ArcaneDriver: DriverEntry\n"));

    // Initialize WDF driver
    WDF_DRIVER_CONFIG_INIT(&config, NULL);
    config.DriverPoolTag = 'narA'; // "Aran" in little-endian
    config.EvtDriverUnload = DriverUnload;

    status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);
    if (!NT_SUCCESS(status)) {
        KdPrint(("ArcaneDriver: WdfDriverCreate failed: 0x%x\n", status));
        return status;
    }

    // Create HID keyboard device directly
    status = VirtualHidKeyboardEvtDeviceAdd(WdfGetDriver(), NULL);
    if (!NT_SUCCESS(status)) {
        KdPrint(("ArcaneDriver: Failed to create HID keyboard device: 0x%x\n", status));
        return status;
    }

    KdPrint(("ArcaneDriver: Loaded successfully\n"));
    return STATUS_SUCCESS;
}


VOID DriverUnload(WDFDRIVER Driver)
{
    NTSTATUS status;
    UNICODE_STRING symbolicLink;
    WDFDEVICE device = NULL;
    PDEVICE_CONTEXT deviceContext = NULL;

    KdPrint(("ArcaneDriver: Unloading\n"));

    // Get the device object (you'll need to store it in the driver context)
    // This assumes you've stored your device in the driver context
    // If you have multiple devices, you'll need to iterate through them

    // If you stored your device in the driver context, you can retrieve it like this:
    // WDFDRIVER driver = WdfGetDriver();
    // device = (WDFDEVICE)WdfObjectGetTypedContext(driver, YOUR_DRIVER_CONTEXT_TYPE)->Device;

    // For now, let's assume you can get your device somehow
    // If you only have one device, you might have stored it in a global variable

    // Delete the symbolic link for the keyboard device
    RtlInitUnicodeString(&symbolicLink, L"\\DosDevices\\ArcaneKeyboard");
    status = IoDeleteSymbolicLink(&symbolicLink);
    if (!NT_SUCCESS(status)) {
        KdPrint(("ArcaneDriver: Failed to delete symbolic link: 0x%x\n", status));
    }

    // If you have a control device, delete its symbolic link too
    RtlInitUnicodeString(&symbolicLink, L"\\DosDevices\\ArcaneDriver");
    status = IoDeleteSymbolicLink(&symbolicLink);
    if (!NT_SUCCESS(status)) {
        KdPrint(("ArcaneDriver: Failed to delete control symbolic link: 0x%x\n", status));
    }

    // If you have access to your device context, free the HID descriptor
    if (device != NULL) {
        deviceContext = GetDeviceContext(device);
        if (deviceContext->HidDescriptor != NULL) {
            ExFreePoolWithTag(deviceContext->HidDescriptor, 'yHkV');
            deviceContext->HidDescriptor = NULL;
        }
    }

    // WDF will handle the rest of the cleanup
    KdPrint(("ArcaneDriver: Unload completed\n"));
}