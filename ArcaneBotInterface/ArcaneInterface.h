#pragma once

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <iostream>
#include <utility>
#include <csignal>
#include <atomic>
#include "json.hpp"
#include <zmq.hpp>

#define MOUSE_CLICK CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define MOUSE_RIGHTCLICK CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define MOUSE_MENUCLICK CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define KEYBOARD_INJECT CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define KEYBOARD_TARGETED_INJECT CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define KEYBOARD_PRESS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define KEYBOARD_RELEASE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)

using json = nlohmann::json;

void SignalHandler(int32_t signal);
using MessageCallback = std::function<void(const std::string&, const std::string&)>;

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

// Add mouse structure
typedef struct _MOUSE_CLICK_REQUEST {
    INT32 x;
    INT32 y;
    UCHAR button;       // 0 = left, 1 = right, 2 = middle -- Or menu option
    ULONG_PTR processId; // Target process ID
} MOUSE_CLICK_REQUEST, * PMOUSE_CLICK_REQUEST;

class ArcaneInterface {
private:
    // Driver handle
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    // ZeroMQ members
    zmq::context_t context_;
    zmq::socket_t socket_;
    std::string endpoint_;
    std::atomic<bool> running_{ false };
    std::atomic<int> message_count_{ 0 };

public:
    ArcaneInterface();
    ~ArcaneInterface();

    // Kernel communication
    bool SendToKernel(DWORD ioctlCode, const void* inputData = nullptr, size_t inputSize = 0,
        void* outputData = nullptr, size_t outputSize = 0, DWORD* bytesReturned = nullptr);
    bool HandleCommand(const std::string& jsonString);
    HANDLE GetDeviceHandle() const { return hDevice; }

    // ZeroMQ server functionality (prefixed with Zmq)
    bool ZmqInitialize(const std::string& port = "5555");
    void ZmqStart(MessageCallback callback = nullptr);
    void ZmqStop();
    bool ZmqIsRunning() const;
    int ZmqGetMessageCount() const;

private:
    void ZmqDefaultMessageProcessing(const std::string& message, const std::string& timestamp);
    std::string GetCurrentTimestamp() const;
};

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