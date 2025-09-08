#pragma once

#ifdef _KERNEL_MODE
#include <wdm.h>
#else
#include <windows.h>
#include <iostream>

void Print() {
    std::cout << std::endl;
}

template<typename T, typename... Args>
void Print(T&& first, Args&&... args) {
    std::cout << std::forward<T>(first);
    if constexpr (sizeof...(args) > 0) {
        std::cout << ' ';  // Add space between arguments
        Print(std::forward<Args>(args)...);  // Recursively process remaining arguments
    }
    else {
        std::cout << std::endl;  // Newline after last argument
    }
}
#endif


const wchar_t DRIVER_DEVICE_PATH[] = L"\\\\.\\ArcaneDriver";

class RuneClient {
private:
    const LONG ClientID;
public:
    RuneClient() = delete;
    ~RuneClient() = delete;
};


// Define structures shared between user and kernel space
#pragma pack(push, 1)  // Ensure no padding
typedef struct _ArcaneClickData {
    LONG x;
    LONG y;
} ArcaneClickData, *PArcaneClickData;

typedef struct _ArcaneKeypressData {
    UCHAR keys[16];  // Fixed size array for keys
    ULONG count;     // Number of valid keys in the array
} ArcaneKeypressData, *PArcanekeypressData;
#pragma pack(pop)


// Define our IOCTL codes
#define ARCANE_DEVICE_TYPE 0x8000
#define ARCANE_CLICK CTL_CODE(ARCANE_DEVICE_TYPE, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define ARCANE_RIGHTCLICK CTL_CODE(ARCANE_DEVICE_TYPE, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define ARCANE_MENUCLICK CTL_CODE(ARCANE_DEVICE_TYPE, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define ARCANE_KEYPRESS CTL_CODE(ARCANE_DEVICE_TYPE, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)