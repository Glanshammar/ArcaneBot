# Axion Device Framework
Axion Device Framework is a modular system designed to emulate Human Interface Devices (HIDs) at the kernel level. It provides a stealthy, low-latency communication layer between user-mode applications and the kernel driver, enabling advanced input simulation and control.

## Overview
The framework consists of two primary components:

**Kernel Driver** – Implements the low-level emulation layer responsible for handling HID simulation at the system level.

**User–Kernel Interface** – Serves as an intermediate middleware for user-mode applications, exposing a clean and secure API to communicate with the driver without direct kernel interaction.

This architecture allows developers to issue high-precision control commands, simulate real input events, and extend functionality modularly for different device types.

## Features
HID simulation through kernel-level emulation.

Current support includes:
* Mouse motion and click events
* Keyboard input and keystroke injection
* Intermediate interface for user–kernel communication.
* Extensible design for adding new device classes (gamepads, touch devices, custom HID profiles).
* Low latency and minimal footprint for real-time interaction.
* Focus on reliability, modularity, and integration with custom automation or testing frameworks.

## Architecture
+-----------------------+
|  User Application     |
+----------+------------+
           |
           v
+-----------------------+
|  User–Kernel Interface|
|  (Intermediate Layer) |
+----------+------------+
           |
           v
+-----------------------+
|    Kernel Driver      |
| (Axion Device Core) |
+-----------------------+
           |
           v
+-----------------------+
|   Virtual HID Device  |
+-----------------------+

## Usage Overview
Load the kernel driver using an appropriate loader or service.
Use the provided user-mode API (PhantomInterface.dll or static library) to connect and send simulation commands.
Applications can issue mouse or keyboard simulation calls, query driver state, and manage virtual devices dynamically.

## Planned Enhancements
Additional HID profiles (joysticks, touchpads, multimedia controllers).
Advanced scripting support.
Secure communication channel options.
Driver configuration tools and diagnostic interface.
