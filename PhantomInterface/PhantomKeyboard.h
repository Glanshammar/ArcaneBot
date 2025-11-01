#pragma once

#include <windows.h>
#include <winioctl.h>
#include <vector>
#include <string>

#define MOUSE_CLICK CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define MOUSE_RIGHTCLICK CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define MOUSE_MENUCLICK CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define KEYBOARD_INJECT CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define KEYBOARD_TARGETED_INJECT CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define KEYBOARD_PRESS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define KEYBOARD_RELEASE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Define the same structures as in your driver
typedef struct _KEY_INJECTION_REQUEST {
    ULONG_PTR ProcessId;        // Target process ID
    ULONG_PTR WindowHandle;     // Target window handle (alternative to ProcessId)
    UCHAR Modifier;
    UCHAR KeyCount;
    UCHAR KeyCodes[6];
} KEY_INJECTION_REQUEST, * PKEY_INJECTION_REQUEST;

class VirtualKeyboard {
private:
    HANDLE hDevice;
    bool isOpen;

public:
    VirtualKeyboard();
    ~VirtualKeyboard();

    bool Open();
    void Close();
    bool IsOpen() const { return isOpen; }

    // Key injection methods
    bool InjectKeys(UCHAR modifier, const std::vector<UCHAR>& keyCodes);
    bool PressKey(UCHAR keyCode);
    bool ReleaseKeys();
    bool TypeString(const std::string& text);
    bool InjectKeysTargeted(ULONG_PTR processId, UCHAR modifier, const std::vector<UCHAR>& keyCodes);
    bool InjectKeysTargeted(HWND windowHandle, UCHAR modifier, const std::vector<UCHAR>& keyCodes);

    // Helper methods
    static UCHAR CharToHidCode(char c);
    static UCHAR VirtualKeyToHidCode(UINT virtualKey);
};