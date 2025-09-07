#include <windows.h>
#include <stdio.h>
#include "ArcaneShared.h"
#include "ArcaneInterface.h"
#include "MQ.h"
#include <iostream>
#include <csignal>
#include <atomic>
#include "json.hpp"

#include <stdexcept>


std::atomic<bool> global_running{ true };

void SignalHandler(int32_t signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nReceived shutdown signal..." << std::endl;
        global_running = false;
    }
}

ArcaneInterface::ArcaneInterface()
    : context_(1), socket_(context_, ZMQ_PULL)
{
    // Open a handle to the driver
    hDevice = CreateFileW(
        DRIVER_DEVICE_PATH,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hDevice == INVALID_HANDLE_VALUE) {
        DWORD errorCode = GetLastError();
        throw std::runtime_error("Failed to open device. Error code: " + std::to_string(errorCode));
    }

    std::cout << "Successfully opened device handle." << std::endl;
}

ArcaneInterface::~ArcaneInterface() {
    if (hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hDevice);
    }
    ZmqStop();
}

// ===== Kernel Communication =====

bool ArcaneInterface::SendToKernel(DWORD ioctlCode, const void* inputData, size_t inputSize,
    void* outputData, size_t outputSize, DWORD* bytesReturned) {
    DWORD localBytesReturned = 0;
    if (bytesReturned == nullptr) {
        bytesReturned = &localBytesReturned;
    }

    BOOL result = DeviceIoControl(
        hDevice,
        ioctlCode,
        const_cast<LPVOID>(inputData),  // Remove const for Windows API
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

bool ArcaneInterface::HandleCommand(const std::string& jsonString) {
    try {
        json j = json::parse(jsonString);
        std::string command = j.at("command");
        auto args = j.at("arguments");

        if (command == "KEYPRESS") {
            std::vector<uint8_t> keys = args.get<std::vector<uint8_t>>();
            return SendToKernel(ARCANE_KEYPRESS, keys.data(), keys.size());
        }
        else if (command == "MENUCLICK") {
            int8_t menu = args.at(0);
            return SendToKernel(ARCANE_MENUCLICK, &menu, sizeof(menu));
        }
        else if (command == "LEFTCLICK") {
            int32_t x = args.at(0);
            int32_t y = args.at(1);
            struct {
                int32_t x;
                int32_t y;
            } clickCoords{ x, y };
            return SendToKernel(ARCANE_CLICK, &clickCoords, sizeof(clickCoords));
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

bool ArcaneInterface::ZmqInitialize(const std::string& port) {
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

void ArcaneInterface::ZmqStart(MessageCallback callback) {
    if (running_) {
        std::cout << "ZeroMQ server already running." << std::endl;
        return;
    }

    running_ = true;
    std::cout << "Starting ZeroMQ server on " << endpoint_ << std::endl;
    std::cout << "Press Ctrl+C to stop the server" << std::endl;

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

void ArcaneInterface::ZmqStop() {
    running_ = false;
}

bool ArcaneInterface::ZmqIsRunning() const {
    return running_;
}

int ArcaneInterface::ZmqGetMessageCount() const {
    return message_count_;
}

void ArcaneInterface::ZmqDefaultMessageProcessing(const std::string& message, const std::string& timestamp) {
    std::cout << "[" << timestamp << "] "
        << "Received message #" << message_count_ << ": "
        << message << std::endl;

    // Example: pass message directly to kernel handling
    HandleCommand(message);
}

std::string ArcaneInterface::GetCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
    return ss.str();
}