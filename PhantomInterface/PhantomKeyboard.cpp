// VirtualKeyboard.cpp
#include "ArcaneKeyboard.h"
#include <iostream>


VirtualKeyboard::VirtualKeyboard() : hDevice(INVALID_HANDLE_VALUE), isOpen(false) {
}

VirtualKeyboard::~VirtualKeyboard() {
    Close();
}

bool VirtualKeyboard::Open() {
    if (isOpen) {
        return true;
    }

    hDevice = CreateFile(
        L"\\\\.\\ArcaneKeyboard",
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hDevice == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open keyboard device. Error: " << GetLastError() << std::endl;
        isOpen = false;
        return false;
    }

    isOpen = true;
    return true;
}

void VirtualKeyboard::Close() {
    if (isOpen) {
        CloseHandle(hDevice);
        hDevice = INVALID_HANDLE_VALUE;
        isOpen = false;
    }
}

bool VirtualKeyboard::InjectKeys(UCHAR modifier, const std::vector<UCHAR>& keyCodes) {
    if (!isOpen) {
        std::cerr << "Keyboard device is not open" << std::endl;
        return false;
    }

    if (keyCodes.size() > 6) {
        std::cerr << "Too many key codes. Maximum is 6." << std::endl;
        return false;
    }

    KEY_INJECTION_REQUEST request;
    request.Modifier = modifier;
    request.KeyCount = static_cast<UCHAR>(keyCodes.size());

    for (size_t i = 0; i < keyCodes.size(); i++) {
        request.KeyCodes[i] = keyCodes[i];
    }

    DWORD bytesReturned;
    BOOL result = DeviceIoControl(
        hDevice,
        KEYBOARD_INJECT,
        &request, sizeof(request),
        NULL, 0,
        &bytesReturned,
        NULL
    );

    if (!result) {
        std::cerr << "Failed to inject keys. Error: " << GetLastError() << std::endl;
        return false;
    }

    return true;
}

bool VirtualKeyboard::InjectKeysTargeted(ULONG_PTR processId, UCHAR modifier, const std::vector<UCHAR>& keyCodes) {
    if (!isOpen) {
        std::cerr << "Keyboard device is not open" << std::endl;
        return false;
    }

    if (keyCodes.size() > 6) {
        std::cerr << "Too many key codes. Maximum is 6." << std::endl;
        return false;
    }

    KEY_INJECTION_REQUEST request;
    request.ProcessId = processId;
    request.WindowHandle = 0; // Not used when ProcessId is specified
    request.Modifier = modifier;
    request.KeyCount = static_cast<UCHAR>(keyCodes.size());

    for (size_t i = 0; i < keyCodes.size(); i++) {
        request.KeyCodes[i] = keyCodes[i];
    }

    DWORD bytesReturned;
    BOOL result = DeviceIoControl(
        hDevice,
        KEYBOARD_INJECT,
        &request, sizeof(request),
        NULL, 0,
        &bytesReturned,
        NULL
    );

    if (!result) {
        std::cerr << "Failed to inject targeted keys. Error: " << GetLastError() << std::endl;
        return false;
    }

    return true;
}

bool VirtualKeyboard::InjectKeysTargeted(HWND windowHandle, UCHAR modifier, const std::vector<UCHAR>& keyCodes) {
    DWORD processId;
    GetWindowThreadProcessId(windowHandle, &processId);
    return InjectKeysTargeted(processId, modifier, keyCodes);
}

bool VirtualKeyboard::PressKey(UCHAR keyCode) {
    return InjectKeys(0, { keyCode });
}

bool VirtualKeyboard::ReleaseKeys() {
    return InjectKeys(0, {});
}

UCHAR VirtualKeyboard::CharToHidCode(char c) {
    // Letters (same for uppercase and lowercase - modifier handles case)
    if (c >= 'a' && c <= 'z') return 0x04 + (c - 'a');
    if (c >= 'A' && c <= 'Z') return 0x04 + (c - 'A');

    // Numbers
    if (c >= '0' && c <= '9') return 0x1E + (c - '0');

    // Function keys (using special codes outside ASCII range)
    if (c == 0xF1) return 0x3A; // F1
    if (c == 0xF2) return 0x3B; // F2
    if (c == 0xF3) return 0x3C; // F3
    if (c == 0xF4) return 0x3D; // F4
    if (c == 0xF5) return 0x3E; // F5
    if (c == 0xF6) return 0x3F; // F6
    if (c == 0xF7) return 0x40; // F7
    if (c == 0xF8) return 0x41; // F8
    if (c == 0xF9) return 0x42; // F9
    if (c == 0xFA) return 0x43; // F10
    if (c == 0xFB) return 0x44; // F11
    if (c == 0xFC) return 0x45; // F12

    // Special characters
    switch (c) {
        // Main keyboard area
    case ' ': return 0x2C; // Space
    case '\n': return 0x28; // Enter (Return)
    case '\t': return 0x2B; // Tab
    case '`': return 0x35; // Grave accent and tilde
    case '~': return 0x35; // Grave accent and tilde (with shift)
    case '-': return 0x2D; // Hyphen and underscore
    case '_': return 0x2D; // Hyphen and underscore (with shift)
    case '=': return 0x2E; // Equal and plus
    case '+': return 0x2E; // Equal and plus (with shift)
    case '[': return 0x2F; // Left bracket and brace
    case '{': return 0x2F; // Left bracket and brace (with shift)
    case ']': return 0x30; // Right bracket and brace
    case '}': return 0x30; // Right bracket and brace (with shift)
    case '\\': return 0x31; // Backslash and vertical bar
    case '|': return 0x31; // Backslash and vertical bar (with shift)
    case ';': return 0x33; // Semicolon and colon
    case ':': return 0x33; // Semicolon and colon (with shift)
    case '\'': return 0x34; // Apostrophe and quotation mark
    case '"': return 0x34; // Apostrophe and quotation mark (with shift)
    case ',': return 0x36; // Comma and less than
    case '<': return 0x36; // Comma and less than (with shift)
    case '.': return 0x37; // Period and greater than
    case '>': return 0x37; // Period and greater than (with shift)
    case '/': return 0x38; // Slash and question mark
    case '?': return 0x38; // Slash and question mark (with shift)

    default: return 0; // Unknown character
    }
}

// Separate function for virtual key codes
UCHAR VirtualKeyboard::VirtualKeyToHidCode(UINT virtualKey) {
    switch (virtualKey) {
        // Navigation keys
        case VK_INSERT: return 0x49; // Insert
        case VK_HOME: return 0x4A; // Home
        case VK_PRIOR: return 0x4B; // Page Up
        case VK_DELETE: return 0x4C; // Delete
        case VK_END: return 0x4D; // End
        case VK_NEXT: return 0x4E; // Page Down
        case VK_RIGHT: return 0x4F; // Right Arrow
        case VK_LEFT: return 0x50; // Left Arrow
        case VK_DOWN: return 0x51; // Down Arrow
        case VK_UP: return 0x52; // Up Arrow

        // Numpad keys
        case VK_NUMLOCK: return 0x53; // Num Lock
        case VK_DIVIDE: return 0x54; // Numpad divide
        case VK_MULTIPLY: return 0x55; // Numpad multiply
        case VK_SUBTRACT: return 0x56; // Numpad subtract
        case VK_ADD: return 0x57; // Numpad add
        case VK_NUMPAD1: return 0x59; // Numpad 1
        case VK_NUMPAD2: return 0x5A; // Numpad 2
        case VK_NUMPAD3: return 0x5B; // Numpad 3
        case VK_NUMPAD4: return 0x5C; // Numpad 4
        case VK_NUMPAD5: return 0x5D; // Numpad 5
        case VK_NUMPAD6: return 0x5E; // Numpad 6
        case VK_NUMPAD7: return 0x5F; // Numpad 7
        case VK_NUMPAD8: return 0x60; // Numpad 8
        case VK_NUMPAD9: return 0x61; // Numpad 9
        case VK_NUMPAD0: return 0x62; // Numpad 0
        case VK_DECIMAL: return 0x63; // Numpad decimal

        // Function keys
        case VK_F1: return 0x3A;
        case VK_F2: return 0x3B;
        case VK_F3: return 0x3C;
        case VK_F4: return 0x3D;
        case VK_F5: return 0x3E;
        case VK_F6: return 0x3F;
        case VK_F7: return 0x40;
        case VK_F8: return 0x41;
        case VK_F9: return 0x42;
        case VK_F10: return 0x43;
        case VK_F11: return 0x44;
        case VK_F12: return 0x45;

        // Other special keys
        case VK_ESCAPE: return 0x29;
        case VK_BACK: return 0x2A;
        case VK_RETURN: return 0x28; // Main enter key
        case VK_CAPITAL: return 0x39; // Caps Lock
        case VK_SCROLL: return 0x47; // Scroll Lock
        case VK_PRINT: return 0x46; // Print Screen
        case VK_PAUSE: return 0x48; // Pause/Break

        // Modifier keys (these are handled separately as modifiers)
        case VK_LSHIFT: return 0x02;
        case VK_RSHIFT: return 0x20;
        case VK_LCONTROL: return 0x01;
        case VK_RCONTROL: return 0x10;
        case VK_LMENU: return 0x04; // Left Alt
        case VK_RMENU: return 0x40; // Right Alt
        case VK_LWIN: return 0x08; // Left Windows
        case VK_RWIN: return 0x80; // Right Windows

        default: return 0; // Unknown virtual key
    }
}

bool VirtualKeyboard::TypeString(const std::string& text) {
    for (char c : text) {
        UCHAR keyCode = CharToHidCode(c);
        if (keyCode == 0) {
            std::cerr << "Unsupported character: " << c << std::endl;
            continue;
        }

        if (!PressKey(keyCode) || !ReleaseKeys()) {
            return false;
        }
    }
    return true;
}