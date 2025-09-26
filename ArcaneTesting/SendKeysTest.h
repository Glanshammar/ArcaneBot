#pragma once

#include "ArcaneKeyboard.h"
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include <tlhelp32.h>

DWORD FindProcessId(const std::wstring& processName) {
    DWORD processId = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(processEntry);

        if (Process32First(snapshot, &processEntry)) {
            do {
                if (processName == processEntry.szExeFile) {
                    processId = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(snapshot, &processEntry));
        }
        CloseHandle(snapshot);
    }

    return processId;
}

struct FoundProcess {
    DWORD processId;
    std::wstring windowTitle;
};

std::vector<FoundProcess> FindProcessesByPartialWindowTitle(const std::wstring& partialTitle) {
    std::vector<FoundProcess> results;

    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        std::vector<FoundProcess>& outResults = *reinterpret_cast<std::vector<FoundProcess>*>(lParam);

        const int length = GetWindowTextLength(hwnd);
        if (length == 0) return TRUE; // no title, continue

        std::wstring title(length + 1, L'\0');
        GetWindowText(hwnd, &title[0], length + 1);
        title.resize(length);

        // Check if window is visible and title contains the partial string
        if (IsWindowVisible(hwnd) && title.find(*reinterpret_cast<const std::wstring*>(lParam + sizeof(std::vector<FoundProcess>))) != std::wstring::npos) {
            DWORD pid = 0;
            GetWindowThreadProcessId(hwnd, &pid);

            FoundProcess fp{ pid, title };
            outResults.push_back(fp);
        }
        return TRUE; // continue enumeration
        }, reinterpret_cast<LPARAM>(&results));

    return results;
}


void TestSendKeys() {
    VirtualKeyboard keyboard;

    if (!keyboard.Open()) {
        std::cerr << "Failed to open keyboard device" << std::endl;
        return;
    }

    // Find a specific process (e.g., Notepad)
    DWORD notepadPid = FindProcessId(L"notepad.exe");

    if (notepadPid == 0) {
        std::cerr << "Notepad not found" << std::endl;
        keyboard.Close();
        return;
    }

    // Type a string into the specific process
    if (keyboard.InjectKeysTargeted(notepadPid, 0, { keyboard.CharToHidCode('H'),
                                                    keyboard.CharToHidCode('e'),
                                                    keyboard.CharToHidCode('l'),
                                                    keyboard.CharToHidCode('l'),
                                                    keyboard.CharToHidCode('o') })) {
        std::cout << "Successfully injected keys into Notepad" << std::endl;
    }
    else {
        std::cerr << "Failed to inject keys" << std::endl;
    }

    keyboard.Close();
    return;
}

void TestSendKeys2() {
    VirtualKeyboard keyboard;
    if (!keyboard.Open()) {
        std::cerr << "Failed to open keyboard device" << std::endl;
        return;
    }
    // Find processes with window titles containing "Notepad"
    auto foundProcesses = FindProcessesByPartialWindowTitle(L"Notepad");
    if (foundProcesses.empty()) {
        std::cerr << "No Notepad windows found" << std::endl;
        keyboard.Close();
        return;
    }
    // Inject keys into each found process
    for (const auto& proc : foundProcesses) {
        std::wcout << L"Injecting keys into process ID: " << proc.processId << L", Window Title: " << proc.windowTitle << std::endl;
        if (keyboard.InjectKeysTargeted(proc.processId, 0, { keyboard.CharToHidCode('H'),
                                                              keyboard.CharToHidCode('e'),
                                                              keyboard.CharToHidCode('l'),
                                                              keyboard.CharToHidCode('l'),
                                                              keyboard.CharToHidCode('o') })) {
            std::cout << "Successfully injected keys" << std::endl;
        }
        else {
            std::cerr << "Failed to inject keys" << std::endl;
        }
    }
    keyboard.Close();
	return;
}