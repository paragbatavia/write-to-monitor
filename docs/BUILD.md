# Build Instructions

## Prerequisites

### Windows
- **Visual Studio 2019 or later** (Community edition is fine)
- **CMake 3.16 or later** - Download from [cmake.org](https://cmake.org/download/)
- **Git** (for submodule management)
- **NVidia GPU and drivers** (for runtime)

### Required Submodules
Make sure the NVidia API submodule is initialized:
```bash
git submodule update --init --recursive
```

## Building the Project

### Option 1: Command Line Build (Recommended)

1. **Open Developer Command Prompt for VS** (or PowerShell with VS environment)

2. **Navigate to project directory:**
   ```bash
   cd path/to/write-to-monitor
   ```

3. **Create and enter build directory:**
   ```bash
   mkdir build
   cd build
   ```

4. **Generate build files:**
   ```bash
   # For 64-bit (recommended)
   cmake .. -A x64
   
   # Or for 32-bit
   cmake .. -A Win32
   ```

5. **Build the project:**
   ```bash
   cmake --build . --config Release
   ```

6. **Find your executable:**
   The built executable will be in `build/bin/Release/writeValueToDisplay.exe`

### Option 2: Visual Studio IDE

1. **Open Visual Studio**
2. **File > Open > CMake...**
3. **Select the `CMakeLists.txt` file in the project root**
4. **Choose your configuration** (x64-Release recommended)
5. **Build > Build All**

### Option 3: VS Code with CMake Tools

1. **Install the CMake Tools extension**
2. **Open the project folder in VS Code**
3. **Ctrl+Shift+P > "CMake: Configure"**
4. **Select your compiler and architecture**
5. **Ctrl+Shift+P > "CMake: Build"**

## Running the Application

```bash
# Basic usage (from build directory)
./bin/Release/writeValueToDisplay.exe [display_index] [input_value] [command_code]

# Example: Set brightness (VCP code 0x10) to 50% on first monitor
./bin/Release/writeValueToDisplay.exe 0 32 10

# With custom register address
./bin/Release/writeValueToDisplay.exe 0 32 10 51
```

## Project Structure

```
write-to-monitor/
├── CMakeLists.txt          # Build configuration
├── README.md              # Project overview
├── src/                   # Source files
│   └── writeValueToDisplay.cpp
├── include/               # Header files
│   └── monitor_control.h
├── docs/                  # Documentation
│   └── BUILD.md          # This file
├── external/              # External dependencies
│   └── nvapi/            # NVidia API submodule
├── cmake/                 # CMake modules (future)
└── build/                 # Build output (created during build)
```

## Troubleshooting

### Common Issues

1. **"NVidia API not found" error:**
   - Make sure you've initialized the git submodule: `git submodule update --init --recursive`

2. **Linking errors:**
   - Ensure you're building with the correct architecture (x64 vs x86)
   - Make sure Visual Studio is properly installed with C++ tools

3. **Runtime errors:**
   - Ensure you have an NVidia GPU with recent drivers
   - Run as administrator if needed for hardware access

4. **CMake configuration errors:**
   - Update CMake to version 3.16 or later
   - Make sure Visual Studio C++ tools are installed

### Getting Help
- Check that all prerequisites are installed
- Verify the external/nvapi submodule exists and has content
- For runtime issues, ensure you have an NVidia GPU with current drivers

