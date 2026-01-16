# Context Dump: HTTP API Server Debug Session

## Session Summary

This document captures the context from debugging sessions for the Monitor Control GUI application's HTTP API server reliability issues.

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

## Current Debug Status (2026-01-15)

**Problem**: After implementing fixes, bind is still failing with WSA error code 0 (no socket-level error).

**Analysis**: WSA error 0 means the failure is happening inside cpp-httplib BEFORE any actual socket operation. Possible causes:
- Static initialization order issue with `WSInit` class
- `getaddrinfo` failing before socket creation
- `is_decommissioned` flag set from previous failed attempt

**Diagnostic code added**:
- Explicit `WSAStartup()` call before bind attempt
- Direct `getaddrinfo()` test to verify address resolution
- Detailed logging at each step

**Next step**: Reboot Windows to clear any stale socket/system state and test again.

---

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

1. **Logger implementation** - Thread-safe file logging with timestamps

2. **Fixed log file path** - Now uses absolute path next to executable:
   ```cpp
   char exe_path[MAX_PATH];
   GetModuleFileNameA(NULL, exe_path, MAX_PATH);
   std::string log_path(exe_path);
   size_t last_slash = log_path.find_last_of("\\/");
   if (last_slash != std::string::npos) {
       log_path = log_path.substr(0, last_slash + 1);
   }
   log_path += "monitor_control.log";
   ```

3. **Fixed `ServerThreadFunc()`** - Now uses two-step binding:
   ```cpp
   // Try to bind first (non-blocking check)
   if (!server.bind_to_port(config.host.c_str(), config.port)) {
       // Signal failure and return
   }
   // Signal success, then start blocking listen
   server.listen_after_bind();
   ```

4. **Fixed `Start()` method** - Now waits for actual bind result:
   ```cpp
   bind_cv.wait_for(lock, std::chrono::seconds(5),
       [this]() { return bind_attempted.load(); });
   ```

5. **Added diagnostic logging** before bind:
   ```cpp
   // WSAStartup verification
   WSADATA wsaData;
   int wsa_init_result = WSAStartup(MAKEWORD(2, 2), &wsaData);

   // getaddrinfo test
   struct addrinfo hints = {}, *result = nullptr;
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;
   int gai_result = getaddrinfo(config.host.c_str(), port_str, &hints, &result);
   ```

6. **Added request logging** to all endpoints (`/api/brightness`, `/api/contrast`, `/api/input`, `/api/status`, `/health`)

### File: `src/monitor_control_gui.cpp`

Added API status indicator:
```cpp
if (g_http_server && g_http_server->IsRunning()) {
    ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "[API: Online]");
} else {
    ImGui::TextColored(ImVec4(0.8f, 0.0f, 0.0f, 1.0f), "[API: Offline]");
}
```

## Files Modified (Full List)

- `include/http_api_server.h` - Added logger class and sync primitives
- `src/http_api_server.cpp` - Major changes for logging, race condition fix, and diagnostics
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
=== Log started at 2026-01-15 23:00:00.000 ===
[2026-01-15 23:00:00.001] [INFO] Starting HTTP API server on 127.0.0.1:45678
[2026-01-15 23:00:00.002] [INFO] WSAStartup succeeded (or was already initialized)
[2026-01-15 23:00:00.003] [INFO] getaddrinfo succeeded for 127.0.0.1:45678
[2026-01-15 23:00:00.004] [INFO] Attempting to bind to 127.0.0.1:45678
[2026-01-15 23:00:00.005] [INFO] Successfully bound to 127.0.0.1:45678, starting to listen
[2026-01-15 23:00:00.006] [INFO] Server started successfully
```

## What to Look For

1. **GUI shows `[API: Online]`** (green) = Server is running correctly
2. **GUI shows `[API: Offline]`** (red) = Server failed to start, check log
3. **Log shows bind failure** = Check WSA error code and diagnostic messages
4. **Log shows requests but no response** = I2C/monitor communication issue
5. **No log file created** = Build issue or permissions problem

## Observed Behavior (Pre-reboot)

Log file showed:
```
=== Log started at 2026-01-15 23:02:33.250 ===
[2026-01-15 23:02:33.251] [INFO] Starting HTTP API server on 127.0.0.1:45678
[2026-01-15 23:02:33.251] [INFO] Attempting to bind to 127.0.0.1:45678
[2026-01-15 23:02:33.252] [ERROR] Failed to bind to 127.0.0.1:45678 - WSA error code: 0
[2026-01-15 23:02:33.252] [ERROR] Server failed to bind - check if port 45678 is in use
```

Port was NOT in use (`netstat` showed nothing on 45678), no zombie processes found.

## Next Steps for Debugging

1. Reboot Windows to clear any stale state
2. Run GUI and check new diagnostic log messages (WSAStartup, getaddrinfo results)
3. If still failing, investigate cpp-httplib internals further
4. Consider alternative: use simple Winsock server instead of cpp-httplib

## cpp-httplib Internals (Reference)

The bind flow in cpp-httplib:
1. `bind_to_port()` calls `bind_internal()`
2. `bind_internal()` checks `is_decommissioned` and `is_valid()` first
3. If those pass, calls `create_server_socket()`
4. `create_server_socket()` calls `detail::create_socket()`
5. `detail::create_socket()` calls `getaddrinfo_with_timeout()` then creates socket

`WSInit` static class should call `WSAStartup()` automatically at load time, but there may be static initialization order issues.
