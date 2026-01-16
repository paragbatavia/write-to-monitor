# Context Dump: HTTP API Server Debug Session

## Session Summary

This document captures the context from a debugging session for the Monitor Control GUI application's HTTP API server reliability issues.

## Problem Statement

StreamDeck commands to switch monitor inputs were intermittently failing. The user suspected port binding issues but had no visibility into what was happening.

## Root Cause Analysis

Three critical issues were identified:

1. **Race Condition in Server Startup**
   - Original code: `running = true` was set BEFORE `server.listen()` in `ServerThreadFunc()`
   - `Start()` method waited only 100ms then checked `running` flag
   - If `listen()` failed after 100ms, `Start()` would return `true` (success) but server wasn't actually running

2. **Invisible Errors**
   - All error messages used `printf()` which goes to console
   - GUI app is built with `WIN32` flag = no console window
   - Errors were literally invisible to users

3. **No Runtime Health Visibility**
   - No way to see if server was actually accepting connections
   - GUI showed "HTTP API listening on..." even when server failed

## Changes Made

### File: `include/http_api_server.h`

Added `ServerLogger` class:
```cpp
class ServerLogger {
public:
    static void Init(const std::string& log_path);
    static void Log(const char* level, const char* format, ...);
    static void Close();
private:
    static std::ofstream log_file;
    static std::mutex log_mutex;
};
```

Added sync primitives to `HttpApiServer`:
```cpp
std::atomic<bool> bind_attempted;
std::atomic<bool> bind_succeeded;
std::condition_variable bind_cv;
std::mutex bind_mutex;
```

### File: `src/http_api_server.cpp`

1. **Logger implementation** (lines 12-61) - Thread-safe file logging with timestamps

2. **Fixed `ServerThreadFunc()`** - Now uses two-step binding:
   ```cpp
   // Try to bind first (non-blocking check)
   if (!server.bind_to_port(config.host.c_str(), config.port)) {
       // Signal failure and return
   }
   // Signal success, then start blocking listen
   server.listen_after_bind();
   ```

3. **Fixed `Start()` method** - Now waits for actual bind result:
   ```cpp
   bind_cv.wait_for(lock, std::chrono::seconds(5),
       [this]() { return bind_attempted.load(); });
   ```

4. **Added request logging** to all endpoints (`/api/brightness`, `/api/contrast`, `/api/input`, `/api/status`, `/health`)

### File: `src/monitor_control_gui.cpp`

Added API status indicator (line 390-396):
```cpp
if (g_http_server && g_http_server->IsRunning()) {
    ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "[API: Online]");
} else {
    ImGui::TextColored(ImVec4(0.8f, 0.0f, 0.0f, 1.0f), "[API: Offline]");
}
```

## Files Modified (Full List)

- `include/http_api_server.h` - Added logger class and sync primitives
- `src/http_api_server.cpp` - Major changes for logging and race condition fix
- `src/monitor_control_gui.cpp` - Added visual status indicator

## Build Commands

```powershell
# From project root
cd build
cmake --build . --config Release

# Or full rebuild
Remove-Item -Recurse -Force build
mkdir build
cd build
cmake .. -A x64
cmake --build . --config Release
```

## Test Commands

```powershell
# Health check
curl http://127.0.0.1:45678/health

# Switch to DisplayPort
curl -X POST http://127.0.0.1:45678/api/input -H "Content-Type: application/json" -d '{"source": 3}'

# Switch to HDMI 1
curl -X POST http://127.0.0.1:45678/api/input -H "Content-Type: application/json" -d '{"source": 1}'

# Check port usage
netstat -ano | findstr :45678
```

## Expected Log Output

After running the app, check `monitor_control.log` next to the executable:

```
=== Log started at 2024-01-15 10:30:45.123 ===
[2024-01-15 10:30:45.124] [INFO] Starting HTTP API server on 127.0.0.1:45678
[2024-01-15 10:30:45.125] [INFO] Attempting to bind to 127.0.0.1:45678
[2024-01-15 10:30:45.126] [INFO] Successfully bound to 127.0.0.1:45678, starting to listen
[2024-01-15 10:30:45.127] [INFO] Server started successfully
```

## What to Look For

1. **GUI shows `[API: Online]`** (green) = Server is running correctly
2. **GUI shows `[API: Offline]`** (red) = Server failed to start, check log
3. **Log shows bind failure** = Another instance or process using port 45678
4. **Log shows requests but no response** = I2C/monitor communication issue
5. **No log file created** = Build issue or permissions problem

## Next Steps for Debugging

If issues persist after these changes:
1. Check `monitor_control.log` for error messages
2. Verify only one instance is running
3. Check if firewall is blocking port 45678
4. Test with `curl` to verify API is responding
5. Check StreamDeck configuration (correct URL, port, JSON format)
