# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a Windows C++ application for controlling monitors via I2C using the NVidia API. It provides both a command-line interface and a GUI application using Dear ImGui with DirectX 11. The project sends VCP (VESA Control Protocol) commands or manufacturer-specific commands to monitors through NVidia's I2C interface.

## Build Commands

### Prerequisites
- Visual Studio 2019 or later with C++ tools
- CMake 3.16 or later
- NVidia GPU with recent drivers
- Git submodules must be initialized

### Initialize Submodules
```bash
git submodule update --init --recursive
```

### Build (Command Line)
```bash
# Create build directory
mkdir build
cd build

# Generate build files (x64 recommended)
cmake .. -A x64

# Build Release configuration
cmake --build . --config Release

# Build Debug configuration
cmake --build . --config Debug
```

### Build Outputs
- Console app: `build/bin/Release/writeValueToDisplay.exe`
- GUI app: `build/bin/Release/monitor_control_gui.exe`

### Running the Console Application
```bash
# Basic syntax
writeValueToDisplay.exe <display_index> <input_value> <command_code> [register_address]

# Examples
writeValueToDisplay.exe 0 0x32 0x10           # Set brightness to 50% (VCP)
writeValueToDisplay.exe 0 0x90 0xF4 0x50      # LG-specific: Switch to HDMI 1
writeValueToDisplay.exe 0 0xD0 0xF4 0x50      # LG-specific: Switch to DisplayPort
```

## Architecture

### Core Components

**I2C Communication Layer** (`src/monitor_control.cpp`, `include/monitor_control.h`)
- `WriteValueToMonitor()`: Main function for sending I2C commands to monitors
- `CalculateI2cChecksum()`: Computes XOR checksum for I2C packets
- Uses NVidia API's `NvAPI_I2CWrite()` for low-level hardware access
- I2C device address: 0x37 (7-bit), shifted to 0x6E for write operations
- Default register address: 0x51 (VCP commands)

**Command-Line Interface** (`src/writeValueToDisplay.cpp`)
- Parses command-line arguments for display index, value, command code, and register
- Initializes NVidia API (`NvAPI_Initialize()`)
- Enumerates displays using `NvAPI_EnumNvidiaDisplayHandle()`
- Maps display handles to GPU handles and output IDs
- Single-shot command execution pattern

**GUI Application** (`src/monitor_control_gui.cpp`)
- ImGui-based interface using DirectX 11 backend
- Manages application state for brightness, contrast, and display selection
- Provides real-time sliders and preset buttons
- Window properties: 450x360px, light theme with Segoe UI font
- Supports multiple displays with dropdown selection
- LG Ultragear-specific input switching buttons (HDMI/DisplayPort/USB-C)

### NVidia API Integration

The project uses NVidia's low-level API (included as git submodule in `external/nvapi/`):
- Architecture-specific libraries: `nvapi64.lib` (x64) or `nvapi.lib` (x86)
- Key API calls:
  - `NvAPI_Initialize()` - Must be called first
  - `NvAPI_EnumNvidiaDisplayHandle()` - Enumerate connected displays
  - `NvAPI_GetPhysicalGPUsFromDisplay()` - Get GPU handle for display
  - `NvAPI_GetAssociatedDisplayOutputId()` - Get output ID for I2C operations
  - `NvAPI_I2CWrite()` - Send I2C data packets

### I2C Packet Structure

The monitor control uses a specific I2C packet format:
```
Byte 0: Device address (0x6E for write)
Byte 1: Register address (default 0x51 for VCP)
Byte 2: 0x84 (0x80 | 4 bytes for "modify value" request)
Byte 3: 0x03 (change value flag)
Byte 4: Command code (e.g., 0x10 for brightness, 0x12 for contrast)
Byte 5: High byte of value (usually 0x00)
Byte 6: Low byte of value (actual value 0-255 or 0-100 depending on monitor)
Byte 7: XOR checksum of all previous bytes
```

### ImGui Integration

The GUI uses Dear ImGui with:
- Win32 platform backend (`imgui_impl_win32.cpp`)
- DirectX 11 renderer backend (`imgui_impl_dx11.cpp`)
- Custom light theme with Windows system colors
- Real-time UI updates without explicit render loops
- Direct3D 11 setup with swap chains and render targets

## Project Structure

```
write-to-monitor/
├── CMakeLists.txt              # Build configuration
├── src/
│   ├── writeValueToDisplay.cpp # CLI entry point
│   ├── monitor_control.cpp     # Core I2C/monitor control logic
│   └── monitor_control_gui.cpp # GUI application (ImGui + DX11)
├── include/
│   └── monitor_control.h       # Monitor control API declarations
├── external/
│   ├── nvapi/                  # NVidia API (git submodule)
│   └── imgui/                  # Dear ImGui library (git submodule)
├── docs/
│   ├── BUILD.md               # Detailed build instructions
│   └── GUI_OPTIONS.md         # GUI framework comparison
└── build/                     # Generated build output (not in repo)
```

## Important Implementation Details

### VCP Command Codes
- `0x10`: Brightness (typically 0-100 range)
- `0x12`: Contrast (typically 0-100 range)
- `0x60`: Input source (standard VCP, but not supported by all monitors)

### LG Ultragear-Specific Commands
Some LG monitors don't support standard VCP input switching. Use manufacturer-specific codes:
- Register: `0x50` (instead of `0x51`)
- Command: `0xF4`
- Values: `0x90` (HDMI 1), `0xD0` (DisplayPort), `0x91` (HDMI 2), `0xD1` (USB-C)

### Display Indexing
- Display index 0 is typically the primary monitor
- Use `mstsc.exe /l` in Windows to see display enumeration
- GUI supports multiple displays via dropdown selection

### Build Configuration Notes
- The project explicitly sets Windows application (`WIN32`) flag for the GUI to prevent console window
- Both console and GUI apps link against the same `monitor_control.cpp` implementation
- CMake automatically selects correct architecture-specific NVidia library
- ImGui sources are compiled directly into the GUI executable

## Common Pitfalls

1. **Submodule initialization**: The NVidia API submodule must be initialized before building
2. **Architecture mismatch**: Ensure CMake architecture (-A x64 or -A Win32) matches your system
3. **Monitor compatibility**: Not all monitors support VCP commands; some require manufacturer-specific codes
4. **Administrator rights**: May be required for I2C hardware access on some systems
5. **NVidia-only**: This tool only works with NVidia GPUs and drivers
6. **Display must be connected to NVidia GPU**: Monitors connected to integrated graphics won't work

## Testing Monitor Commands

When implementing new monitor commands:
1. Test with VCP standard codes first (register `0x51`)
2. If VCP doesn't work, research manufacturer-specific codes
3. Values are typically hexadecimal but can be decimal depending on context
4. Always verify checksum calculation for I2C packets
5. Monitor responses are not read back in current implementation

## GUI Development Notes

The GUI application demonstrates:
- Immediate-mode UI pattern with ImGui
- Stateful application design with global `AppState` struct
- Real-time monitor control without blocking UI
- Error handling and status messages
- Theme customization for Windows integration
