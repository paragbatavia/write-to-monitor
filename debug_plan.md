# Debug Plan: HTTP API Server Reliability Issues

## Problem Summary
StreamDeck commands to switch monitor inputs intermittently fail. Root cause: **silent server startup failures** - the HTTP server may fail to bind to the port, but the GUI reports success anyway due to a race condition.

## Critical Issues Identified

1. **Race condition in startup**: `running = true` was set BEFORE `server.listen()` completes
2. **Invisible errors**: `printf()` output goes nowhere (WIN32 GUI has no console)
3. **No health visibility**: No way to verify server is accepting connections

---

## Changes Already Implemented

All code changes have been made. You just need to build and test.

### Files Modified

| File | Changes |
|------|---------|
| `include/http_api_server.h` | Added `ServerLogger` class, added sync primitives (`bind_attempted`, `bind_succeeded`, `bind_cv`, `bind_mutex`) |
| `src/http_api_server.cpp` | Added logger implementation, fixed `Start()` race condition using `bind_to_port()` + `listen_after_bind()`, added request logging to all endpoints |
| `src/monitor_control_gui.cpp` | Added colored API status indicator `[API: Online]` / `[API: Offline]` |

---

## Build Instructions

```powershell
cd build
cmake --build . --config Release
```

Or regenerate build files first:
```powershell
mkdir build -Force
cd build
cmake .. -A x64
cmake --build . --config Release
```

---

## Verification Steps

1. **Build and run** the application
2. **Check log file** - `monitor_control.log` should appear next to the executable
3. **Verify GUI** - Look for green `[API: Online]` or red `[API: Offline]` indicator
4. **Test with curl**:
   ```powershell
   curl http://127.0.0.1:45678/health
   curl -X POST http://127.0.0.1:45678/api/input -H "Content-Type: application/json" -d '{"source": 3}'
   ```
5. **Test port conflict** - Run two instances; second should show `[API: Offline]` and log the error
6. **Test StreamDeck** - Press buttons and check log for request entries

---

## Log File Format

The log file `monitor_control.log` will contain entries like:

```
=== Log started at 2024-01-15 10:30:45.123 ===
[2024-01-15 10:30:45.124] [INFO] Starting HTTP API server on 127.0.0.1:45678
[2024-01-15 10:30:45.125] [INFO] Attempting to bind to 127.0.0.1:45678
[2024-01-15 10:30:45.126] [INFO] Successfully bound to 127.0.0.1:45678, starting to listen
[2024-01-15 10:30:45.127] [INFO] Server started successfully
[2024-01-15 10:31:00.500] [INFO] POST /api/input - body: {"source": 3}
[2024-01-15 10:31:00.501] [INFO] Switching input to DisplayPort (source=3)
[2024-01-15 10:31:00.650] [INFO] SetInputSource(3) = success
```

If binding fails:
```
[2024-01-15 10:30:45.126] [ERROR] Failed to bind to 127.0.0.1:45678 - port may be in use
[2024-01-15 10:30:45.127] [ERROR] Server failed to bind - check if port 45678 is in use
```

---

## Debugging Common Issues

### Port Already in Use
```powershell
netstat -ano | findstr :45678
```
This shows which process is using the port.

### Check if Server is Running
```powershell
curl http://127.0.0.1:45678/health
```
Should return: `{"status": "ok", "version": "1.0.0"}`

### Multiple Instances
Check Task Manager for multiple `monitor_control_gui.exe` processes.

---

## Key Code Changes Summary

### Race Condition Fix
- Changed from `server.listen()` (blocking, can't detect success before it blocks)
- To `server.bind_to_port()` + `server.listen_after_bind()` (can detect bind success before blocking)
- `Start()` now waits on a condition variable for the bind result with 5-second timeout

### Logging
- All API requests logged with timestamps
- Server startup/shutdown logged
- Bind success/failure logged with clear error messages
