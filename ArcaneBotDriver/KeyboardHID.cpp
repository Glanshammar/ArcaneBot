#include "DriverFunctions.h"

// HID Report Descriptor for a standard keyboard
const UCHAR g_KeyboardHidReportDescriptor[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x05, 0x07,        //   Usage Page (Key Codes)
    0x19, 0xE0,        //   Usage Minimum (224)
    0x29, 0xE7,        //   Usage Maximum (231)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data, Variable, Absolute) ; Modifier bytes
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x03,        //   Input (Constant, Variable, Absolute) ; Reserved byte
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x01,        //   Report Size (1)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x19, 0x01,        //   Usage Minimum (1)
    0x29, 0x05,        //   Usage Maximum (5)
    0x91, 0x02,        //   Output (Data, Variable, Absolute) ; LED report
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x03,        //   Report Size (3)
    0x91, 0x03,        //   Output (Constant, Variable, Absolute) ; Padding
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x65,        //   Logical Maximum (101)
    0x05, 0x07,        //   Usage Page (Key Codes)
    0x19, 0x00,        //   Usage Minimum (0)
    0x29, 0x65,        //   Usage Maximum (101)
    0x81, 0x00,        //   Input (Data, Array) ; Key arrays
    0xC0               // End Collection
};

NTSTATUS VirtualHidKeyboardEvtDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit)
{
    NTSTATUS status;
    WDFDEVICE device;
    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    PDEVICE_CONTEXT deviceContext;
    WDF_IO_QUEUE_CONFIG queueConfig;
    UNICODE_STRING deviceName, symbolicLink;

    UNREFERENCED_PARAMETER(Driver);

    KdPrint(("ArcaneKeyboard: ArcaneKeyboardEvtDeviceAdd\n"));

    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_KEYBOARD);
    WdfDeviceInitSetExclusive(DeviceInit, FALSE);

    RtlInitUnicodeString(&deviceName, L"\\Device\\ArcaneKeyboard");
    status = WdfDeviceInitAssignName(DeviceInit, &deviceName);
    if (!NT_SUCCESS(status)) {
        KdPrint(("ArcaneKeyboard: WdfDeviceInitAssignName failed: 0x%x\n", status));
        return status;
    }

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);
    deviceAttributes.EvtCleanupCallback = VirtualHidKeyboardEvtDeviceContextCleanup;

    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);
    if (!NT_SUCCESS(status)) {
        KdPrint(("ArcaneKeyboard: WdfDeviceCreate failed: 0x%x\n", status));
        return status;
    }

    deviceContext = GetDeviceContext(device);
    deviceContext->Device = device;
	deviceContext->DeviceType = DEVICE_TYPE_KEYBOARD;
    deviceContext->HasPendingReport = FALSE;
    RtlZeroMemory(&deviceContext->CurrentInputReport, sizeof(KEYBOARD_INPUT_REPORT));

    RtlInitUnicodeString(&symbolicLink, L"\\DosDevices\\ArcaneKeyboard");
    status = WdfDeviceCreateSymbolicLink(device, &symbolicLink);
    if (!NT_SUCCESS(status)) {
        KdPrint(("ArcaneKeyboard: WdfDeviceCreateSymbolicLink failed: 0x%x\n", status));
        return status;
    }

    status = WdfDeviceCreateDeviceInterface(device, &ARCANE_KEYBOARD_GUID, NULL);
    if (!NT_SUCCESS(status)) {
        KdPrint(("ArcaneKeyboard: WdfDeviceCreateDeviceInterface failed: 0x%x\n", status));
        return status;
    }

    deviceContext->HidDeviceAttributes.Size = sizeof(HID_DEVICE_ATTRIBUTES);
    deviceContext->HidDeviceAttributes.VendorID = 0x1234;
    deviceContext->HidDeviceAttributes.ProductID = 0x5678;
    deviceContext->HidDeviceAttributes.VersionNumber = 0x0100;

    deviceContext->HidDescriptor = (PHID_DESCRIPTOR)ExAllocatePool2(
        POOL_FLAG_NON_PAGED,
        sizeof(HID_DESCRIPTOR),
        'yHkV');

    if (!deviceContext->HidDescriptor) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    deviceContext->HidDescriptor->bLength = sizeof(HID_DESCRIPTOR);
    deviceContext->HidDescriptor->bDescriptorType = HID_HID_DESCRIPTOR_TYPE;
    deviceContext->HidDescriptor->bcdHID = 0x0100;
    deviceContext->HidDescriptor->bCountry = 0;
    deviceContext->HidDescriptor->bNumDescriptors = 1;
    deviceContext->HidDescriptor->DescriptorList[0].bReportType = HID_REPORT_DESCRIPTOR_TYPE;
    deviceContext->HidDescriptor->DescriptorList[0].wReportLength = sizeof(g_KeyboardHidReportDescriptor);

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);
    queueConfig.EvtIoDeviceControl = VirtualHidKeyboardEvtIoDeviceControl;
    queueConfig.EvtIoInternalDeviceControl = VirtualHidKeyboardEvtIoInternalDeviceControl;
    queueConfig.EvtIoRead = VirtualHidKeyboardEvtIoRead;

    status = WdfIoQueueCreate(device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &deviceContext->WdfQueue);
    if (!NT_SUCCESS(status)) {
        KdPrint(("ArcaneKeyboard: WdfIoQueueCreate failed: 0x%x\n", status));
        return status;
    }

    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);
    status = WdfIoQueueCreate(device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &deviceContext->PendingReadQueue);
    if (!NT_SUCCESS(status)) {
        KdPrint(("ArcaneKeyboard: WdfIoQueueCreate for pending reads failed: 0x%x\n", status));
        return status;
    }

    deviceContext->IsInitialized = TRUE;
    KdPrint(("ArcaneKeyboard: Device created successfully\n"));

    return status;
}

VOID VirtualHidKeyboardEvtDeviceContextCleanup(WDFOBJECT DeviceObject)
{
    PDEVICE_CONTEXT deviceContext = GetDeviceContext((WDFDEVICE)DeviceObject);

    KdPrint(("ArcaneKeyboard: VirtualHidKeyboardEvtDeviceContextCleanup\n"));

    if (deviceContext->HidDescriptor != NULL) {
        ExFreePoolWithTag(deviceContext->HidDescriptor, 'yHkV');
        deviceContext->HidDescriptor = NULL;
    }
}

VOID VirtualHidKeyboardEvtIoDeviceControl(
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

    KdPrint(("ArcaneKeyboard: VirtualHidKeyboardEvtIoDeviceControl - IoControlCode: 0x%x\n", IoControlCode));

    switch (IoControlCode) {
    case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
        status = VirtualHidKeyboardGetDeviceDescriptor(deviceContext, Request, &bytesReturned);
        break;

    case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
        status = VirtualHidKeyboardGetDeviceAttributes(deviceContext, Request, &bytesReturned);
        break;

    case IOCTL_HID_GET_REPORT_DESCRIPTOR:
        status = VirtualHidKeyboardGetReportDescriptor(deviceContext, Request, &bytesReturned);
        break;

    case IOCTL_HID_GET_STRING:
        status = VirtualHidKeyboardGetString(Request, &bytesReturned);
        break;

    case IOCTL_HID_READ_REPORT:
    case IOCTL_HID_WRITE_REPORT:
    case IOCTL_HID_GET_FEATURE:
    case IOCTL_HID_SET_FEATURE:
        status = VirtualHidKeyboardHandleReport(deviceContext, Request, IoControlCode, &bytesReturned);
        break;
    case KEYBOARD_INJECT:
        status = VirtualHidKeyboardInjectKey(deviceContext, Request, &bytesReturned);
        break;

    case KEYBOARD_PRESS:
        status = VirtualHidKeyboardPressKey(deviceContext, Request, &bytesReturned);
        break;

    case KEYBOARD_RELEASE:
        status = VirtualHidKeyboardReleaseKeys(deviceContext, Request, &bytesReturned);
        break;

    default:
        status = STATUS_NOT_SUPPORTED;
        break;
    }

    WdfRequestCompleteWithInformation(Request, status, bytesReturned);
}

VOID VirtualHidKeyboardEvtIoInternalDeviceControl(
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength,
    ULONG IoControlCode)
{
    VirtualHidKeyboardEvtIoDeviceControl(Queue, Request, OutputBufferLength, InputBufferLength, IoControlCode);
}

VOID VirtualHidKeyboardEvtIoRead(
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t Length)
{
    WDFDEVICE device = WdfIoQueueGetDevice(Queue);
    PDEVICE_CONTEXT deviceContext = GetDeviceContext(device);
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(Length);

    KdPrint(("ArcaneKeyboard: VirtualHidKeyboardEvtIoRead\n"));

    if (deviceContext->HasPendingReport) {
        // We have a report to send, complete the request immediately
        status = VirtualHidKeyboardCompleteReadRequest(deviceContext, Request);
    }
    else {
        // No report available, queue the request for later completion
        status = WdfRequestForwardToIoQueue(Request, deviceContext->PendingReadQueue);
        if (!NT_SUCCESS(status)) {
            WdfRequestCompleteWithInformation(Request, status, 0);
        }
    }
}

NTSTATUS VirtualHidKeyboardGetDeviceDescriptor(
    PDEVICE_CONTEXT DeviceContext,
    WDFREQUEST Request,
    size_t* BytesReturned)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDFMEMORY memory;

    status = WdfRequestRetrieveOutputMemory(Request, &memory);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = WdfMemoryCopyFromBuffer(
        memory,
        0,
        DeviceContext->HidDescriptor,
        sizeof(HID_DESCRIPTOR));

    if (NT_SUCCESS(status)) {
        *BytesReturned = sizeof(HID_DESCRIPTOR);
    }

    return status;
}

NTSTATUS VirtualHidKeyboardGetDeviceAttributes(
    PDEVICE_CONTEXT DeviceContext,
    WDFREQUEST Request,
    size_t* BytesReturned)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDFMEMORY memory;

    status = WdfRequestRetrieveOutputMemory(Request, &memory);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = WdfMemoryCopyFromBuffer(
        memory,
        0,
        &DeviceContext->HidDeviceAttributes,
        sizeof(HID_DEVICE_ATTRIBUTES));

    if (NT_SUCCESS(status)) {
        *BytesReturned = sizeof(HID_DEVICE_ATTRIBUTES);
    }

    return status;
}

NTSTATUS VirtualHidKeyboardGetReportDescriptor(
    PDEVICE_CONTEXT DeviceContext,
    WDFREQUEST Request,
    size_t* BytesReturned)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDFMEMORY memory;

    UNREFERENCED_PARAMETER(DeviceContext);

    status = WdfRequestRetrieveOutputMemory(Request, &memory);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Fix: Cast g_KeyboardHidReportDescriptor to PVOID
    status = WdfMemoryCopyFromBuffer(
        memory,
        0,
        (PVOID)g_KeyboardHidReportDescriptor,
        sizeof(g_KeyboardHidReportDescriptor));

    if (NT_SUCCESS(status)) {
        *BytesReturned = sizeof(g_KeyboardHidReportDescriptor);
    }

    return status;
}

NTSTATUS VirtualHidKeyboardGetString(
    WDFREQUEST Request,
    size_t* BytesReturned)
{
    UNREFERENCED_PARAMETER(Request);
    *BytesReturned = 0;
    return STATUS_SUCCESS;
}

NTSTATUS VirtualHidKeyboardHandleReport(
    PDEVICE_CONTEXT DeviceContext,
    WDFREQUEST Request,
    ULONG IoControlCode,
    size_t* BytesReturned)
{
    NTSTATUS status = STATUS_SUCCESS;

    switch (IoControlCode) {
    case IOCTL_HID_READ_REPORT:
        if (DeviceContext->HasPendingReport) {
            status = VirtualHidKeyboardCompleteReadRequest(DeviceContext, Request);
        }
        else {
            status = WdfRequestForwardToIoQueue(Request, DeviceContext->PendingReadQueue);
            if (!NT_SUCCESS(status)) {
                *BytesReturned = 0;
            }
        }
        break;

    case IOCTL_HID_WRITE_REPORT:
    case IOCTL_HID_GET_FEATURE:
    case IOCTL_HID_SET_FEATURE:
    default:
        status = STATUS_NOT_SUPPORTED;
        *BytesReturned = 0;
        break;
    }

    return status;
}

NTSTATUS VirtualHidKeyboardCompleteReadRequest(
    PDEVICE_CONTEXT DeviceContext,
    WDFREQUEST Request)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDFMEMORY memory;
    size_t bytesReturned = 0;

    status = WdfRequestRetrieveOutputMemory(Request, &memory);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = WdfMemoryCopyFromBuffer(
        memory,
        0,
        &DeviceContext->CurrentInputReport,
        sizeof(KEYBOARD_INPUT_REPORT));

    if (NT_SUCCESS(status)) {
        bytesReturned = sizeof(KEYBOARD_INPUT_REPORT);
        DeviceContext->HasPendingReport = FALSE;
    }

    WdfRequestCompleteWithInformation(Request, status, bytesReturned);
    return status;
}

NTSTATUS VirtualHidKeyboardInjectKey(
    PDEVICE_CONTEXT DeviceContext,
    WDFREQUEST Request,
    size_t* BytesReturned)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDFMEMORY memory;
    KEY_INJECTION_REQUEST injectionRequest;
    WDFREQUEST pendingRequest = NULL;
    PEPROCESS targetProcess = NULL;
    KAPC_STATE apcState = { 0 };
    BOOLEAN processAttached = FALSE;

    status = WdfRequestRetrieveInputMemory(Request, &memory);
    if (!NT_SUCCESS(status)) {
        KdPrint(("ArcaneKeyboard: Failed to retrieve input memory: 0x%x\n", status));
        return status;
    }

    status = WdfMemoryCopyToBuffer(
        memory,
        0,
        &injectionRequest,
        sizeof(KEY_INJECTION_REQUEST));

    if (!NT_SUCCESS(status)) {
        KdPrint(("ArcaneKeyboard: Failed to copy request buffer: 0x%x\n", status));
        return status;
    }

    // Validate the request
    if (injectionRequest.KeyCount > 6) {
        KdPrint(("ArcaneKeyboard: Too many key codes: %d\n", injectionRequest.KeyCount));
        return STATUS_INVALID_PARAMETER;
    }

    KdPrint(("ArcaneKeyboard: InjectKey - ProcessId: %lu, Modifier: 0x%x, KeyCount: %d\n",
        injectionRequest.ProcessId, injectionRequest.Modifier, injectionRequest.KeyCount));

    // If a process ID was specified, attach to that process
    if (injectionRequest.ProcessId != 0) {
        status = PsLookupProcessByProcessId((HANDLE)injectionRequest.ProcessId, &targetProcess);
        if (!NT_SUCCESS(status)) {
            KdPrint(("ArcaneKeyboard: Failed to find process with ID: %lu, status: 0x%x\n",
                injectionRequest.ProcessId, status));
            return status;
        }

        // Attach to the target process context
        KeStackAttachProcess(targetProcess, &apcState);
        processAttached = TRUE;
        KdPrint(("ArcaneKeyboard: Attached to process: %lu\n", injectionRequest.ProcessId));
    }

    // Prepare the input report
    if (DeviceContext->DeviceType == DEVICE_TYPE_KEYBOARD) {
        RtlZeroMemory(&DeviceContext->CurrentInputReport.KeyboardReport, sizeof(KEYBOARD_INPUT_REPORT));
        DeviceContext->CurrentInputReport.KeyboardReport.Modifier = injectionRequest.Modifier;

        for (UCHAR i = 0; i < injectionRequest.KeyCount; i++) {
            DeviceContext->CurrentInputReport.KeyboardReport.KeyCode[i] = injectionRequest.KeyCodes[i];
            KdPrint(("ArcaneKeyboard: Key[%d]: 0x%x\n", i, injectionRequest.KeyCodes[i]));
        }
    }
    else {
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto Cleanup;
    }

    DeviceContext->HasPendingReport = TRUE;

    // Check if there are any pending read requests
    status = WdfIoQueueRetrieveNextRequest(DeviceContext->PendingReadQueue, &pendingRequest);
    if (NT_SUCCESS(status)) {
        KdPrint(("ArcaneKeyboard: Completing pending read request\n"));
        VirtualHidKeyboardCompleteReadRequest(DeviceContext, pendingRequest);
    }

Cleanup:
    // If we attached to a process, detach now
    if (processAttached) {
        KeUnstackDetachProcess(&apcState);
        ObDereferenceObject(targetProcess);
        KdPrint(("ArcaneKeyboard: Detached from process\n"));
    }

    *BytesReturned = 0;
    return status;
}


NTSTATUS VirtualHidKeyboardPressKey(
    PDEVICE_CONTEXT DeviceContext,
    WDFREQUEST Request,
    size_t* BytesReturned)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDFMEMORY memory;
    SINGLE_KEY_REQUEST keyRequest;
    WDFREQUEST pendingRequest = NULL;

    status = WdfRequestRetrieveInputMemory(Request, &memory);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = WdfMemoryCopyToBuffer(
        memory,
        0,
        &keyRequest,
        sizeof(SINGLE_KEY_REQUEST));

    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Prepare the input report with a single key
    DeviceContext->CurrentInputReport.KeyboardReport.Modifier = keyRequest.Modifier;
    DeviceContext->CurrentInputReport.KeyboardReport.Reserved = 0;

    if (DeviceContext->DeviceType == DEVICE_TYPE_KEYBOARD) {
        RtlZeroMemory(DeviceContext->CurrentInputReport.KeyboardReport.KeyCode, sizeof(DeviceContext->CurrentInputReport.KeyboardReport.KeyCode));
        DeviceContext->CurrentInputReport.KeyboardReport.KeyCode[0] = keyRequest.KeyCode;
    }
    else {
		return STATUS_INVALID_DEVICE_REQUEST;
	}

    DeviceContext->HasPendingReport = TRUE;

    // If targeted injection, attach to the process
    if (keyRequest.ProcessId != 0) {
        PEPROCESS targetProcess = NULL;
        KAPC_STATE apcState;

        status = PsLookupProcessByProcessId((HANDLE)keyRequest.ProcessId, &targetProcess);
        if (NT_SUCCESS(status)) {
            KeStackAttachProcess(targetProcess, &apcState);

            // Complete any pending read requests in the context of the target process
            status = WdfIoQueueRetrieveNextRequest(DeviceContext->PendingReadQueue, &pendingRequest);
            if (NT_SUCCESS(status)) {
                VirtualHidKeyboardCompleteReadRequest(DeviceContext, pendingRequest);
            }

            KeUnstackDetachProcess(&apcState);
            ObDereferenceObject(targetProcess);
        }
    }
    else {
        // Complete any pending read requests
        status = WdfIoQueueRetrieveNextRequest(DeviceContext->PendingReadQueue, &pendingRequest);
        if (NT_SUCCESS(status)) {
            VirtualHidKeyboardCompleteReadRequest(DeviceContext, pendingRequest);
        }
    }

    *BytesReturned = 0;
    return status;
}

NTSTATUS VirtualHidKeyboardReleaseKeys(
    PDEVICE_CONTEXT DeviceContext,
    WDFREQUEST Request,
    size_t* BytesReturned)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDFREQUEST pendingRequest = NULL;
    ULONG_PTR processId = 0;
    WDFMEMORY memory;

    // Check if we have a process ID in the request
    status = WdfRequestRetrieveInputMemory(Request, &memory);
    if (NT_SUCCESS(status)) {
        status = WdfMemoryCopyToBuffer(
            memory,
            0,
            &processId,
            sizeof(ULONG_PTR));
    }

    // Prepare an empty input report (no keys pressed)
    RtlZeroMemory(&DeviceContext->CurrentInputReport, sizeof(KEYBOARD_INPUT_REPORT));
    DeviceContext->HasPendingReport = TRUE;

    // If targeted release, attach to the process
    if (processId != 0) {
        PEPROCESS targetProcess = NULL;
        KAPC_STATE apcState;

        status = PsLookupProcessByProcessId((HANDLE)processId, &targetProcess);
        if (NT_SUCCESS(status)) {
            KeStackAttachProcess(targetProcess, &apcState);

            // Complete any pending read requests in the context of the target process
            status = WdfIoQueueRetrieveNextRequest(DeviceContext->PendingReadQueue, &pendingRequest);
            if (NT_SUCCESS(status)) {
                VirtualHidKeyboardCompleteReadRequest(DeviceContext, pendingRequest);
            }

            KeUnstackDetachProcess(&apcState);
            ObDereferenceObject(targetProcess);
        }
    }
    else {
        // Complete any pending read requests
        status = WdfIoQueueRetrieveNextRequest(DeviceContext->PendingReadQueue, &pendingRequest);
        if (NT_SUCCESS(status)) {
            VirtualHidKeyboardCompleteReadRequest(DeviceContext, pendingRequest);
        }
    }

    *BytesReturned = 0;
    return status;
}