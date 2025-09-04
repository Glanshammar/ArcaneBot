#include <windows.h>
#include <stdio.h>
#include "ArcaneShared.h"

int main()
{
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    BOOL result = FALSE;
    DWORD bytesReturned = 0;
    ArcaneData data;

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
        printf("Failed to open device. Error: %d\n", GetLastError());
        return 1;
    }

    Print("Successfully opened device handle.");

    data.Number = 100;
    wcscpy_s(data.Message, L"Hello Driver! (from library)");
    Print("Received from driver: Value=%d, Message=%ws", data.Number, data.Message);

    // Modify the data and send it back to the driver
    data.Number = 420;
    Print("Changed data number to ", data.Number);

    result = DeviceIoControl(
        hDevice,
        ARCANE_HELLO,
        &data, sizeof(data),   // Pass ArcaneData as data
        NULL, 0,   // No output data
        &bytesReturned,
        NULL
    );

    if (!result) {
        Print("Failed to set data in driver. Error: %d", GetLastError());
        CloseHandle(hDevice);
        return 1;
    }

    Print("Successfully sent data to driver");
    CloseHandle(hDevice);
    return 0;
}