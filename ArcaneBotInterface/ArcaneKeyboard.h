#pragma once

#include <windows.h>
#include <vector>
#include <string>

// Define the same structures as in your driver
typedef struct _KEY_INJECTION_REQUEST {
    ULONG_PTR ProcessId;        // Target process ID
    ULONG_PTR WindowHandle;     // Target window handle (alternative to ProcessId)
    UCHAR Modifier;
    UCHAR KeyCount;
    UCHAR KeyCodes[50];
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