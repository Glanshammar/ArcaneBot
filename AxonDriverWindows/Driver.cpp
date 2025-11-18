#include "DriverFunctions.h"

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

    KdPrint(("AxonDriver: DriverEntry\n"));

    // Initialize WDF driver
    WDF_DRIVER_CONFIG_INIT(&config, NULL);
    config.DriverPoolTag = 'narA'; // "Aran" in little-endian
    config.EvtDriverUnload = DriverUnload;

    status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);
    if (!NT_SUCCESS(status)) {
        KdPrint(("AxonDriver: WdfDriverCreate failed: 0x%x\n", status));
        return status;
    }

    KdPrint(("AxonDriver: Loaded successfully\n"));
    return STATUS_SUCCESS;
}


VOID DriverUnload(WDFDRIVER Driver)
{
	UNREFERENCED_PARAMETER(Driver);

    NTSTATUS status;
    UNICODE_STRING symbolicLink;
    WDFDEVICE device = NULL;
    PDEVICE_CONTEXT deviceContext = NULL;

    KdPrint(("AxonDriver: Unloading\n"));

    // Delete the symbolic link for the keyboard device
    RtlInitUnicodeString(&symbolicLink, L"\\DosDevices\\AxonKeyboard");
    status = IoDeleteSymbolicLink(&symbolicLink);
    if (!NT_SUCCESS(status)) {
        KdPrint(("AxonDriver: Failed to delete symbolic link: 0x%x\n", status));
    }

    // If you have a control device, delete its symbolic link too
    RtlInitUnicodeString(&symbolicLink, L"\\DosDevices\\AxonDriver");
    status = IoDeleteSymbolicLink(&symbolicLink);
    if (!NT_SUCCESS(status)) {
        KdPrint(("AxonDriver: Failed to delete control symbolic link: 0x%x\n", status));
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
    KdPrint(("AxonDriver: Unload completed\n"));
}