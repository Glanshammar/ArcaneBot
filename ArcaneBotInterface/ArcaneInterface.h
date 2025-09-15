#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdio.h>
#include "ArcaneShared.h"
#include <iostream>
#include <utility>
#include <csignal>
#include <atomic>
#include "json.hpp"
#include <zmq.hpp>

using json = nlohmann::json;

void SignalHandler(int32_t signal);
using MessageCallback = std::function<void(const std::string&, const std::string&)>;

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

#define ARCANE_DEVICE_TYPE 0x8000
#define ARCANE_CLICK CTL_CODE(ARCANE_DEVICE_TYPE, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define ARCANE_RIGHTCLICK CTL_CODE(ARCANE_DEVICE_TYPE, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define ARCANE_MENUCLICK CTL_CODE(ARCANE_DEVICE_TYPE, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define ARCANE_KEYPRESS CTL_CODE(ARCANE_DEVICE_TYPE, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)