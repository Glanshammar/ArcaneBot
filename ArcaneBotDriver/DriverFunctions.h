#pragma once

#include <ntifs.h>
#include <wdm.h>
#include <wdf.h>
#include <hidport.h>
#include <hidsdi.h>
#include <initguid.h>
#include "ArcaneShared.h"

// GUID for virtual keyboard device interface
DEFINE_GUID(ARCANE_KEYBOARD_GUID,
    0x12345678, 0x1234, 0x1234, 0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef);


#define MOUSE_CLICK CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define MOUSE_RIGHTCLICK CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define MOUSE_MENUCLICK CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define KEYBOARD_INJECT CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define KEYBOARD_TARGETED_INJECT CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define KEYBOARD_PRESS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define KEYBOARD_RELEASE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define DEVICE_TYPE_KEYBOARD 0
#define DEVICE_TYPE_MOUSE 1

extern const UCHAR g_KeyboardHidReportDescriptor[];

// Structure for key injection requests
typedef struct _KEY_INJECTION_REQUEST {
    ULONG_PTR ProcessId;        // Target process ID
    ULONG_PTR WindowHandle;     // Target window handle (alternative to ProcessId)
    UCHAR Modifier;
    UCHAR KeyCount;
    UCHAR KeyCodes[6];
} KEY_INJECTION_REQUEST, * PKEY_INJECTION_REQUEST;

// Structure for single key press
typedef struct _SINGLE_KEY_REQUEST {
    ULONG_PTR ProcessId;
    UCHAR KeyCode;
    UCHAR Modifier;
} SINGLE_KEY_REQUEST, * PSINGLE_KEY_REQUEST;

// Keyboard input report structure
typedef struct _KEYBOARD_INPUT_REPORT {
    UCHAR Modifier;
    UCHAR Reserved;
    UCHAR KeyCode[6];
} KEYBOARD_INPUT_REPORT, * PKEYBOARD_INPUT_REPORT;

// Device context structure
typedef struct _DEVICE_CONTEXT {
    WDFDEVICE Device;
    HID_DEVICE_ATTRIBUTES HidDeviceAttributes;
    PHID_DESCRIPTOR HidDescriptor;
    WDFQUEUE WdfQueue;
    WDFQUEUE PendingReadQueue;
    union {
        KEYBOARD_INPUT_REPORT KeyboardReport;
        // MOUSE_INPUT_REPORT MouseReport; // Add this when you implement mouse
    } CurrentInputReport;
    BOOLEAN HasPendingReport;
    BOOLEAN IsInitialized;
    UCHAR DeviceType;  // 0 = keyboard, 1 = mouse
} DEVICE_CONTEXT, * PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, GetDeviceContext)

// Function declarations
NTSTATUS VirtualHidKeyboardEvtDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit);
VOID VirtualHidKeyboardEvtDeviceContextCleanup(WDFOBJECT DeviceObject);
VOID VirtualHidKeyboardEvtIoDeviceControl(WDFQUEUE Queue, WDFREQUEST Request, size_t OutputBufferLength,
    size_t InputBufferLength, ULONG IoControlCode);
VOID VirtualHidKeyboardEvtIoInternalDeviceControl(WDFQUEUE Queue, WDFREQUEST Request, size_t OutputBufferLength,
    size_t InputBufferLength, ULONG IoControlCode);
VOID VirtualHidKeyboardEvtIoRead(WDFQUEUE Queue, WDFREQUEST Request, size_t Length);
NTSTATUS VirtualHidKeyboardGetDeviceDescriptor(PDEVICE_CONTEXT DeviceContext, WDFREQUEST Request, size_t* BytesReturned);
NTSTATUS VirtualHidKeyboardGetDeviceAttributes(PDEVICE_CONTEXT DeviceContext, WDFREQUEST Request, size_t* BytesReturned);
NTSTATUS VirtualHidKeyboardGetReportDescriptor(PDEVICE_CONTEXT DeviceContext, WDFREQUEST Request, size_t* BytesReturned);
NTSTATUS VirtualHidKeyboardGetString(WDFREQUEST Request, size_t* BytesReturned);
NTSTATUS VirtualHidKeyboardHandleReport(PDEVICE_CONTEXT DeviceContext, WDFREQUEST Request,
    ULONG IoControlCode, size_t* BytesReturned);
NTSTATUS VirtualHidKeyboardCompleteReadRequest(PDEVICE_CONTEXT DeviceContext, WDFREQUEST Request);
NTSTATUS VirtualHidKeyboardInjectKey(PDEVICE_CONTEXT DeviceContext, WDFREQUEST Request, size_t* BytesReturned);
NTSTATUS VirtualHidKeyboardTargetedInjectKey(PDEVICE_CONTEXT DeviceContext, WDFREQUEST Request, size_t* BytesReturned);
NTSTATUS VirtualHidKeyboardPressKey(PDEVICE_CONTEXT DeviceContext, WDFREQUEST Request, size_t* BytesReturned);
NTSTATUS VirtualHidKeyboardReleaseKeys(PDEVICE_CONTEXT DeviceContext, WDFREQUEST Request, size_t* BytesReturned);