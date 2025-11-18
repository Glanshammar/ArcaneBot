#pragma once

#include <windows.h>
#include <winioctl.h>
#include <string>

#define MOUSE_CLICK CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define MOUSE_RIGHTCLICK CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define MOUSE_MENUCLICK CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define KEYBOARD_INJECT CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define KEYBOARD_TARGETED_INJECT CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define KEYBOARD_PRESS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define KEYBOARD_RELEASE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)


typedef struct _KEY_INJECTION_REQUEST {
    ULONG_PTR ProcessId;
    ULONG_PTR WindowHandle;
    UCHAR Modifier;
    UCHAR KeyCount;
    UCHAR KeyCodes[6];
} KEY_INJECTION_REQUEST, * PKEY_INJECTION_REQUEST;

typedef struct _SINGLE_KEY_REQUEST {
    ULONG_PTR ProcessId;
    UCHAR KeyCode;
    UCHAR Modifier;
} SINGLE_KEY_REQUEST, * PSINGLE_KEY_REQUEST;

typedef struct _MOUSE_CLICK_REQUEST {
    INT32 x;
    INT32 y;
    UCHAR button;       // 0 = left, 1 = right, 2 = middle -- Or menu option
    ULONG_PTR processId; // Target process ID
} MOUSE_CLICK_REQUEST, * PMOUSE_CLICK_REQUEST;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PHANTOMINTERFACE_EXPORTS
#define AXON_API __declspec(dllexport)
#else
#define AXON_API __declspec(dllimport)
#endif

    // Initialization
    AXON_API int initialize();
    AXON_API void shutdown();
    AXON_API int is_initialized();

    // Keyboard operations
    AXON_API int key_press(unsigned char key_code, unsigned char modifier, unsigned long process_id);
    AXON_API int key_release(unsigned long process_id);
    AXON_API int type_string(const char* text, unsigned long process_id);
    AXON_API int key_inject(unsigned char modifier, const unsigned char* key_codes, unsigned char key_count, unsigned long process_id);

    // Mouse operations
    AXON_API int left_click(int x, int y, unsigned long process_id);
    AXON_API int right_click(int x, int y, unsigned long process_id);
    AXON_API int menu_click(int x, int y, unsigned long process_id);

    // Helper functions
    AXON_API unsigned char char_to_hid_code(char c);
    AXON_API unsigned char vk_to_hid_code(unsigned int virtual_key);
#ifdef __cplusplus
}
#endif

AXON_API std::string GetCurrentTimestamp();

inline void Print() {
    std::cout << std::endl;
}

template<typename T, typename... Args>
inline void Print(T&& first, Args&&... args) {
    std::cout << std::forward<T>(first);
    if constexpr (sizeof...(args) > 0) {
        std::cout << ' ';
        Print(std::forward<Args>(args)...);
    }
    else {
        std::cout << std::endl;
    }
}