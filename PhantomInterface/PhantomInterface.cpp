#define PHANTOMINTERFACE_EXPORTS

#include "PhantomInterface.h"
#include <iostream>
#include <mutex>
#include <vector>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>


namespace {
    HANDLE g_hDevice = INVALID_HANDLE_VALUE;
    std::mutex g_deviceMutex;

    bool SendIoctl(DWORD ioctlCode, const void* inputData = nullptr,
        size_t inputSize = 0, void* outputData = nullptr,
        size_t outputSize = 0) {
        std::lock_guard<std::mutex> lock(g_deviceMutex);

        if (g_hDevice == INVALID_HANDLE_VALUE) {
            return false;
        }

        DWORD bytesReturned = 0;
        BOOL result = DeviceIoControl(
            g_hDevice,
            ioctlCode,
            const_cast<LPVOID>(inputData),
            static_cast<DWORD>(inputSize),
            outputData,
            static_cast<DWORD>(outputSize),
            &bytesReturned,
            NULL
        );

        if (!result) {
            DWORD error = GetLastError();
            std::cerr << "DeviceIoControl failed. Error: " << error << " for IOCTL: " << ioctlCode << std::endl;
            return false;
        }

        return true;
    }
}

// ===== C API Implementation =====
PHANTOM_API int initialize() {
    std::lock_guard<std::mutex> lock(g_deviceMutex);

    if (g_hDevice != INVALID_HANDLE_VALUE) {
        return 1; // Already initialized
    }

    g_hDevice = CreateFileW(
        L"\\\\.\\ArcaneKeyboard",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (g_hDevice == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        std::cerr << "Failed to open keyboard device. Error: " << error << std::endl;
        return 0;
    }

    std::cout << "PhantomInterface initialized successfully." << std::endl;
    return 1;
}

PHANTOM_API void shutdown() {
    std::lock_guard<std::mutex> lock(g_deviceMutex);

    if (g_hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(g_hDevice);
        g_hDevice = INVALID_HANDLE_VALUE;
    }
}

PHANTOM_API int is_initialized() {
    std::lock_guard<std::mutex> lock(g_deviceMutex);
    return (g_hDevice != INVALID_HANDLE_VALUE) ? 1 : 0;
}

// Keyboard operations
PHANTOM_API int key_press(unsigned char key_code, unsigned char modifier, unsigned long process_id) {
    SINGLE_KEY_REQUEST request = { 0 };
    request.KeyCode = key_code;
    request.Modifier = modifier;
    request.ProcessId = process_id;

    return SendIoctl(KEYBOARD_PRESS, &request, sizeof(request)) ? 1 : 0;
}

PHANTOM_API int key_release(unsigned long process_id) {
    ULONG_PTR pid = process_id;
    return SendIoctl(KEYBOARD_RELEASE, &pid, sizeof(pid)) ? 1 : 0;
}

PHANTOM_API int key_inject(unsigned char modifier, const unsigned char* key_codes, unsigned char key_count, unsigned long process_id) {
    if (key_count > 6) {
        std::cerr << "Too many key codes. Maximum is 6." << std::endl;
        return 0;
    }

    KEY_INJECTION_REQUEST request = { 0 };
    request.ProcessId = process_id;
    request.Modifier = modifier;
    request.KeyCount = key_count;

    for (unsigned char i = 0; i < key_count; i++) {
        request.KeyCodes[i] = key_codes[i];
    }

    DWORD ioctl = process_id ? KEYBOARD_TARGETED_INJECT : KEYBOARD_INJECT;
    return SendIoctl(ioctl, &request, sizeof(request)) ? 1 : 0;
}

PHANTOM_API int type_string(const char* text, unsigned long process_id) {
    if (!text) return 0;

    std::string str(text);
    for (char c : str) {
        unsigned char key_code = char_to_hid_code(c);
        if (key_code == 0) {
            std::cerr << "Unsupported character: " << c << std::endl;
            continue;
        }

        if (!key_press(key_code, 0, process_id) || !key_release(process_id)) {
            return 0;
        }
    }
    return 1;
}

// Mouse operations
PHANTOM_API int left_click(int x, int y, unsigned long process_id) {
    MOUSE_CLICK_REQUEST request = { 0 };
    request.x = x;
    request.y = y;
    request.button = 0; // left button
    request.processId = process_id;

    return SendIoctl(MOUSE_CLICK, &request, sizeof(request)) ? 1 : 0;
}

PHANTOM_API int right_click(int x, int y, unsigned long process_id) {
    MOUSE_CLICK_REQUEST request = { 0 };
    request.x = x;
    request.y = y;
    request.button = 1; // right button
    request.processId = process_id;

    return SendIoctl(MOUSE_RIGHTCLICK, &request, sizeof(request)) ? 1 : 0;
}

PHANTOM_API int menu_click(int x, int y, unsigned long process_id) {
    MOUSE_CLICK_REQUEST request = { 0 };
    request.x = x;
    request.y = y;
    request.button = 2; // menu button
    request.processId = process_id;

    return SendIoctl(MOUSE_MENUCLICK, &request, sizeof(request)) ? 1 : 0;
}

// Keep your existing conversion functions
PHANTOM_API unsigned char char_to_hid_code(char c) {
    // Your existing implementation from PhantomKeyboard.cpp
    if (c >= 'a' && c <= 'z') return 0x04 + (c - 'a');
    if (c >= 'A' && c <= 'Z') return 0x04 + (c - 'A');
    if (c >= '0' && c <= '9') return 0x1E + (c - '0');

    switch (c) {
    case ' ': return 0x2C;
    case '\n': return 0x28;
    case '\t': return 0x2B;
    case '`': return 0x35;
    case '~': return 0x35;
    case '-': return 0x2D;
    case '_': return 0x2D;
    case '=': return 0x2E;
    case '+': return 0x2E;
    case '[': return 0x2F;
    case '{': return 0x2F;
    case ']': return 0x30;
    case '}': return 0x30;
    case '\\': return 0x31;
    case '|': return 0x31;
    case ';': return 0x33;
    case ':': return 0x33;
    case '\'': return 0x34;
    case '"': return 0x34;
    case ',': return 0x36;
    case '<': return 0x36;
    case '.': return 0x37;
    case '>': return 0x37;
    case '/': return 0x38;
    case '?': return 0x38;
    default: return 0;
    }
}

PHANTOM_API unsigned char vk_to_hid_code(unsigned int virtual_key) {
    // Your existing implementation from PhantomKeyboard.cpp
    switch (virtual_key) {
    case VK_INSERT: return 0x49;
    case VK_HOME: return 0x4A;
    case VK_PRIOR: return 0x4B;
    case VK_DELETE: return 0x4C;
    case VK_END: return 0x4D;
    case VK_NEXT: return 0x4E;
    case VK_RIGHT: return 0x4F;
    case VK_LEFT: return 0x50;
    case VK_DOWN: return 0x51;
    case VK_UP: return 0x52;
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
    case VK_ESCAPE: return 0x29;
    case VK_BACK: return 0x2A;
    case VK_RETURN: return 0x28;
    default: return 0;
    }
}

PHANTOM_API std::string GetCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
    return ss.str();
}