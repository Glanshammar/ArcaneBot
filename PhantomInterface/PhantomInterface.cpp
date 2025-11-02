#include <windows.h>

#include <stdio.h>
#include <iostream>
#include <csignal>
#include <atomic>
#include <stdexcept>

#include "PhantomInterface.h"
#include "json.hpp"

std::atomic<bool> global_running{ true };

void SignalHandler(int32_t signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nReceived shutdown signal..." << std::endl;
        global_running = false;
    }
}

PhantomInterface::PhantomInterface()
    : context_(1), socket_(context_, ZMQ_PULL)
{
    // Open a handle to the keyboard device directly
    hDevice = CreateFileW(
        L"\\\\.\\ArcaneKeyboard",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hDevice == INVALID_HANDLE_VALUE) {
        DWORD errorCode = GetLastError();
        throw std::runtime_error("Failed to open keyboard device. Error code: " + std::to_string(errorCode));
    }

    std::cout << "Successfully opened keyboard device handle." << std::endl;
}

PhantomInterface::~PhantomInterface() {
    if (hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hDevice);
    }
    ZmqStop();
}

// ===== Kernel Communication =====
bool PhantomInterface::SendToKernel(DWORD ioctlCode, const void* inputData, size_t inputSize,
    void* outputData, size_t outputSize, DWORD* bytesReturned) {
    DWORD localBytesReturned = 0;
    if (bytesReturned == nullptr) {
        bytesReturned = &localBytesReturned;
    }

    BOOL result = DeviceIoControl(
        hDevice,
        ioctlCode,
        const_cast<LPVOID>(inputData),
        static_cast<DWORD>(inputSize),
        outputData,
        static_cast<DWORD>(outputSize),
        bytesReturned,
        NULL);

    if (!result) {
        DWORD error = GetLastError();
        std::cerr << "DeviceIoControl failed with error " << error << " for IOCTL " << ioctlCode << std::endl;
        return false;
    }

    return true;
}

bool PhantomInterface::HandleCommand(const std::string& jsonString) {
    try {
        json jsonData = json::parse(jsonString);
        std::string command = jsonData.at("command");
        auto args = jsonData.at("arguments");

        if (command == "KEYPRESS") {
            SINGLE_KEY_REQUEST keyRequest = { 0 };
            keyRequest.KeyCode = args.at(0);        // Key code
            keyRequest.Modifier = args.at(1);       // Modifier
            keyRequest.ProcessId = args.at(2);      // Target process ID

            return SendToKernel(KEYBOARD_PRESS, &keyRequest, sizeof(keyRequest));
        }
        else if (command == "KEYINJECT") {
            KEY_INJECTION_REQUEST injectRequest = { 0 };
            injectRequest.Modifier = args.at(0);    // Modifier
            injectRequest.ProcessId = args.at(1);   // Target process ID

            auto keyCodes = args.at(2); // Array of key codes
            injectRequest.KeyCount = static_cast<UCHAR>(keyCodes.size());

            for (size_t i = 0; i < keyCodes.size() && i < 6; i++) {
                injectRequest.KeyCodes[i] = keyCodes.at(i);
            }

            return SendToKernel(KEYBOARD_INJECT, &injectRequest, sizeof(injectRequest));
        }
        else if (command == "KEYRELEASE") {
            ULONG_PTR processId = args.at(0); // Target process ID for release
            return SendToKernel(KEYBOARD_RELEASE, &processId, sizeof(processId));
        }
        else if (command == "LEFTCLICK") {
            MOUSE_CLICK_REQUEST clickData = {
                args.at(0),  // x
                args.at(1),  // y
                0,           // left button
                args.at(2)   // process ID
            };
            return SendToKernel(MOUSE_CLICK, &clickData, sizeof(clickData));
        }
        else if (command == "RIGHTCLICK") {
            MOUSE_CLICK_REQUEST clickData = {
                args.at(0),  // x
                args.at(1),  // y  
                1,           // right button
                args.at(2)   // process ID
            };
            return SendToKernel(MOUSE_RIGHTCLICK, &clickData, sizeof(clickData));
        }

        std::cerr << "Unknown command: " << command << std::endl;
        return false;
    }
    catch (const std::exception& e) {
        std::cerr << "JSON parsing or handling error: " << e.what() << std::endl;
        return false;
    }
}

// ===== ZeroMQ =====
bool PhantomInterface::ZmqInitialize(const std::string& port) {
    try {
        endpoint_ = "tcp://*:" + port;
        socket_.bind(endpoint_);
        std::cout << "ZeroMQ server initialized and bound to: " << endpoint_ << std::endl;
        return true;
    }
    catch (const zmq::error_t& e) {
        std::cerr << "Failed to initialize ZeroMQ server: " << e.what() << std::endl;
        return false;
    }
}

void PhantomInterface::ZmqStart(MessageCallback callback) {
    if (running_) {
        std::cout << "ZeroMQ server is already running." << std::endl;
        return;
    }

    running_ = true;
    std::cout << "Starting ZeroMQ server on " << endpoint_ << "." << std::endl;
    std::cout << "Press Ctrl+C to stop the server." << std::endl;

    MessageCallback processor = callback;
    if (!processor) {
        processor = [this](const std::string& msg, const std::string& ts) {
            this->ZmqDefaultMessageProcessing(msg, ts);
            };
    }

    zmq::pollitem_t items[] = { {socket_, 0, ZMQ_POLLIN, 0} };

    while (running_ && global_running) {
        zmq::poll(items, 1, 100);
        if (items[0].revents & ZMQ_POLLIN) {
            zmq::message_t message;
            auto recv_result = socket_.recv(message, zmq::recv_flags::none);
            if (recv_result) {
                message_count_++;
                std::string message_str(static_cast<char*>(message.data()), message.size());
                std::string timestamp = GetCurrentTimestamp();
                processor(message_str, timestamp);
            }
        }
    }

    std::cout << "ZeroMQ server stopped." << std::endl;
}

void PhantomInterface::ZmqStop() {
    running_ = false;
}

bool PhantomInterface::ZmqIsRunning() const {
    return running_;
}

int PhantomInterface::ZmqGetMessageCount() const {
    return message_count_;
}

void PhantomInterface::ZmqDefaultMessageProcessing(const std::string& message, const std::string& timestamp) {
    std::cout << "[" << timestamp << "] "
        << "Received message #" << message_count_ << ": "
        << message << std::endl;

    // Example: pass message directly to kernel handling
    HandleCommand(message);
}

std::string PhantomInterface::GetCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
    return ss.str();
}