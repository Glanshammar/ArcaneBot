#pragma once

#include "ArcaneKeyboard.h"


void TestSendKeys() {
    VirtualKeyboard keyboard;

    if (keyboard.Open()) {
        // Find the target window
        HWND targetWindow = FindWindow(NULL, L"Notepad");
        if (targetWindow) {
            // Get the process ID
            DWORD processId;
            GetWindowThreadProcessId(targetWindow, &processId);

            // Type text into the specific window
            keyboard.InjectKeysTargeted(processId, 0, { 
                keyboard.CharToHidCode('h'),
                keyboard.CharToHidCode('e'),
                keyboard.CharToHidCode('l'),
                keyboard.CharToHidCode('l'), 
                keyboard.CharToHidCode('o'),
				' ',
				'W',
				'o',
				'r',
				'l',
				'd'
                });
            keyboard.InjectKeysTargeted(processId, 0, {});     // Release keys
        }
        keyboard.Close();
    }
}