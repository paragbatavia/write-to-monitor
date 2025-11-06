# Monitor Control HTTP API Documentation

## Overview

The Monitor Control GUI application includes a built-in HTTP REST API server for programmatic control of monitor settings. This API allows you to control brightness, contrast, and input sources remotely through standard HTTP requests.

## Quick Start

1. **Start the GUI application**: Run `monitor_control_gui.exe`
2. **Verify the server is running**: Check the status bar in the GUI - it should display "HTTP API listening on 127.0.0.1:45678"
3. **Test the API**:
   ```bash
   curl http://localhost:45678/health
   ```

## Configuration

### Configuration File

Create or edit `config.env` in the same directory as `monitor_control_gui.exe`:

```ini
# HTTP server port (default: 45678)
HTTP_PORT=45678

# HTTP server host (default: 127.0.0.1 for localhost-only access)
# Warning: Use 0.0.0.0 only if you need external access and understand the security implications
HTTP_HOST=127.0.0.1

# Enable or disable the HTTP API server
# Values: true, false, 1, 0, yes, no, on, off
API_ENABLED=true
```

### Default Settings

If `config.env` is not found, the following defaults are used:
- Port: `45678`
- Host: `127.0.0.1` (localhost-only)
- Enabled: `true`

## Base URL

```
http://localhost:45678
```

## Authentication

**None.** The API is designed for localhost-only access by default. No authentication is required or supported in the current version.

⚠️ **Security Note**: The API binds to `127.0.0.1` (localhost) by default, making it accessible only from the same machine. If you need remote access, consider implementing a reverse proxy with authentication (e.g., nginx, Apache) rather than exposing the API directly.

## API Endpoints

### 1. Set Brightness

Set the monitor brightness level.

**Endpoint:** `POST /api/brightness`

**Request Body:**
```json
{
  "value": 75
}
```

**Parameters:**
| Parameter | Type | Required | Range | Description |
|-----------|------|----------|-------|-------------|
| value | number | Yes | 0-100 | Brightness level (0 = minimum, 100 = maximum) |

**Success Response (200 OK):**
```json
{
  "success": true,
  "message": "Brightness set successfully",
  "brightness": 75
}
```

**Error Responses:**

*400 Bad Request - Invalid value:*
```json
{
  "success": false,
  "message": "Value must be between 0 and 100"
}
```

*400 Bad Request - Missing field:*
```json
{
  "success": false,
  "message": "Invalid request: missing or invalid 'value' field"
}
```

*503 Service Unavailable - NvAPI not initialized:*
```json
{
  "success": false,
  "message": "NvAPI not initialized"
}
```

*500 Internal Server Error - Monitor control failed:*
```json
{
  "success": false,
  "message": "Failed to set brightness"
}
```

**Example:**
```bash
# Set brightness to 75%
curl -X POST http://localhost:45678/api/brightness \
  -H "Content-Type: application/json" \
  -d '{"value": 75}'

# Set brightness to minimum
curl -X POST http://localhost:45678/api/brightness \
  -H "Content-Type: application/json" \
  -d '{"value": 0}'

# Set brightness to maximum
curl -X POST http://localhost:45678/api/brightness \
  -H "Content-Type: application/json" \
  -d '{"value": 100}'
```

---

### 2. Set Contrast

Set the monitor contrast level.

**Endpoint:** `POST /api/contrast`

**Request Body:**
```json
{
  "value": 50
}
```

**Parameters:**
| Parameter | Type | Required | Range | Description |
|-----------|------|----------|-------|-------------|
| value | number | Yes | 0-100 | Contrast level (0 = minimum, 100 = maximum) |

**Success Response (200 OK):**
```json
{
  "success": true,
  "message": "Contrast set successfully",
  "contrast": 50
}
```

**Error Responses:**

Same error response format as `/api/brightness` endpoint.

**Example:**
```bash
# Set contrast to 50%
curl -X POST http://localhost:45678/api/contrast \
  -H "Content-Type: application/json" \
  -d '{"value": 50}'
```

---

### 3. Set Input Source

Switch the monitor input source. This uses LG Ultragear-specific commands.

**Endpoint:** `POST /api/input`

**Request Body:**
```json
{
  "source": 1
}
```

**Parameters:**
| Parameter | Type | Required | Range | Description |
|-----------|------|----------|-------|-------------|
| source | number | Yes | 1-4 | Input source:<br>1 = HDMI 1<br>2 = HDMI 2<br>3 = DisplayPort<br>4 = USB-C |

**Input Source Mapping:**
| Value | Input Name | Description |
|-------|------------|-------------|
| 1 | HDMI 1 | First HDMI port |
| 2 | HDMI 2 | Second HDMI port |
| 3 | DisplayPort | DisplayPort connection |
| 4 | USB-C | USB-C with DisplayPort Alt Mode |

**Success Response (200 OK):**
```json
{
  "success": true,
  "message": "Input switched successfully",
  "input": 1,
  "input_name": "HDMI 1"
}
```

**Error Responses:**

*400 Bad Request - Invalid source:*
```json
{
  "success": false,
  "message": "Source must be between 1 and 4 (1=HDMI 1, 2=HDMI 2, 3=DisplayPort, 4=USB-C)"
}
```

*400 Bad Request - Missing field:*
```json
{
  "success": false,
  "message": "Invalid request: missing or invalid 'source' field"
}
```

*503 Service Unavailable:*
```json
{
  "success": false,
  "message": "NvAPI not initialized"
}
```

*500 Internal Server Error:*
```json
{
  "success": false,
  "message": "Failed to switch input"
}
```

**Example:**
```bash
# Switch to HDMI 1
curl -X POST http://localhost:45678/api/input \
  -H "Content-Type: application/json" \
  -d '{"source": 1}'

# Switch to DisplayPort
curl -X POST http://localhost:45678/api/input \
  -H "Content-Type: application/json" \
  -d '{"source": 3}'

# Switch to USB-C
curl -X POST http://localhost:45678/api/input \
  -H "Content-Type: application/json" \
  -d '{"source": 4}'
```

**Note:** Input switching uses LG Ultragear-specific commands. Other monitor brands may not support these commands. See the main README for details on monitor compatibility.

---

### 4. Get Status

Retrieve the current monitor status and settings.

**Endpoint:** `GET /api/status`

**Request:** No body required.

**Success Response (200 OK):**
```json
{
  "brightness": 75,
  "contrast": 50,
  "display_index": 0,
  "nvapi_initialized": true,
  "status_message": "HTTP API listening on 127.0.0.1:45678"
}
```

**Response Fields:**
| Field | Type | Description |
|-------|------|-------------|
| brightness | number | Current brightness level (0-100) |
| contrast | number | Current contrast level (0-100) |
| display_index | number | Currently selected display index (0 = first display) |
| nvapi_initialized | boolean | Whether NVidia API is successfully initialized |
| status_message | string | Latest status or error message from the application |

**Example:**
```bash
curl http://localhost:45678/api/status
```

**Response:**
```json
{
  "brightness": 75,
  "contrast": 50,
  "display_index": 0,
  "nvapi_initialized": true,
  "status_message": "Brightness set to 75% via API"
}
```

---

### 5. Health Check

Simple health check endpoint to verify the API server is running.

**Endpoint:** `GET /health`

**Request:** No body required.

**Success Response (200 OK):**
```json
{
  "status": "ok",
  "version": "1.0.0"
}
```

**Example:**
```bash
curl http://localhost:45678/health
```

**Use Case:** Use this endpoint for monitoring scripts or to verify the server is responsive before making control requests.

---

## HTTP Status Codes

| Code | Meaning | When Used |
|------|---------|-----------|
| 200 | OK | Request succeeded |
| 400 | Bad Request | Invalid parameters or malformed JSON |
| 500 | Internal Server Error | Monitor control operation failed |
| 503 | Service Unavailable | NVidia API not initialized or monitor not available |

---

## Content Type

All requests and responses use JSON format with the content type:
```
Content-Type: application/json
```

---

## Rate Limiting

**None.** The API does not implement rate limiting. However, monitor I2C operations are inherently slow (50-200ms per command), which naturally limits the effective request rate.

---

## Concurrent Requests

The API is **thread-safe** and can handle concurrent requests. However:
- Only one I2C operation can be in progress at a time (protected by mutex)
- Concurrent requests will be queued and processed sequentially
- GUI interactions and API requests share the same mutex

---

## Integration Examples

### PowerShell

```powershell
# Set brightness
$body = @{ value = 80 } | ConvertTo-Json
Invoke-RestMethod -Uri "http://localhost:45678/api/brightness" `
                  -Method Post `
                  -ContentType "application/json" `
                  -Body $body

# Get status
$status = Invoke-RestMethod -Uri "http://localhost:45678/api/status"
Write-Host "Current brightness: $($status.brightness)%"
```

### Python

```python
import requests

# Set brightness
response = requests.post(
    'http://localhost:45678/api/brightness',
    json={'value': 75}
)
print(response.json())

# Set input source
response = requests.post(
    'http://localhost:45678/api/input',
    json={'source': 3}  # DisplayPort
)
print(response.json())

# Get current status
status = requests.get('http://localhost:45678/api/status').json()
print(f"Brightness: {status['brightness']}%")
print(f"Contrast: {status['contrast']}%")
```

### JavaScript (Node.js)

```javascript
const fetch = require('node-fetch');

// Set brightness
async function setBrightness(value) {
  const response = await fetch('http://localhost:45678/api/brightness', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ value })
  });
  return await response.json();
}

// Get status
async function getStatus() {
  const response = await fetch('http://localhost:45678/api/status');
  return await response.json();
}

// Usage
setBrightness(75).then(console.log);
getStatus().then(console.log);
```

### Batch Script

```batch
@echo off
REM Set brightness to 50%
curl -X POST http://localhost:45678/api/brightness ^
  -H "Content-Type: application/json" ^
  -d "{\"value\": 50}"

REM Switch to HDMI 1
curl -X POST http://localhost:45678/api/input ^
  -H "Content-Type: application/json" ^
  -d "{\"source\": 1}"
```

### AutoHotkey

```autohotkey
; Increase brightness by 10%
^Up::  ; Ctrl+Up
{
    ; Get current brightness
    WinHttp := ComObjCreate("WinHttp.WinHttpRequest.5.1")
    WinHttp.Open("GET", "http://localhost:45678/api/status", false)
    WinHttp.Send()
    status := WinHttp.ResponseText

    ; Parse and increase (simplified - use JSON parser in production)
    RegExMatch(status, "brightness"": (\d+)", match)
    newBrightness := Min(match1 + 10, 100)

    ; Set new brightness
    WinHttp.Open("POST", "http://localhost:45678/api/brightness", false)
    WinHttp.SetRequestHeader("Content-Type", "application/json")
    WinHttp.Send("{""value"": " . newBrightness . "}")
    return
}
```

---

## Use Cases

### 1. Time-Based Brightness Adjustment

Schedule brightness changes based on time of day:

```python
import schedule
import requests

def set_day_mode():
    requests.post('http://localhost:45678/api/brightness', json={'value': 90})
    requests.post('http://localhost:45678/api/contrast', json={'value': 60})
    print("Day mode activated")

def set_night_mode():
    requests.post('http://localhost:45678/api/brightness', json={'value': 30})
    requests.post('http://localhost:45678/api/contrast', json={'value': 40})
    print("Night mode activated")

schedule.every().day.at("08:00").do(set_day_mode)
schedule.every().day.at("20:00").do(set_night_mode)

while True:
    schedule.run_pending()
    time.sleep(60)
```

### 2. Application-Triggered Input Switching

Switch inputs when specific applications launch:

```python
import psutil
import requests
import time

def check_and_switch():
    for proc in psutil.process_iter(['name']):
        if proc.info['name'] == 'game.exe':
            # Switch to HDMI 1 for gaming console
            requests.post('http://localhost:45678/api/input', json={'source': 1})
            break
        elif proc.info['name'] == 'work.exe':
            # Switch to DisplayPort for PC
            requests.post('http://localhost:45678/api/input', json={'source': 3})
            break

while True:
    check_and_switch()
    time.sleep(5)
```

### 3. Home Automation Integration

Control monitor with Home Assistant or similar:

```yaml
# Home Assistant configuration.yaml
rest_command:
  set_monitor_brightness:
    url: http://192.168.1.100:45678/api/brightness
    method: POST
    content_type: "application/json"
    payload: '{"value": {{ brightness }}}'

automation:
  - alias: "Adjust monitor brightness at sunset"
    trigger:
      platform: sun
      event: sunset
    action:
      service: rest_command.set_monitor_brightness
      data:
        brightness: 30
```

---

## Troubleshooting

### Server Not Starting

**Problem:** API server doesn't start, status shows "Failed to start HTTP API server"

**Solutions:**
1. Check if port 45678 is already in use:
   ```bash
   netstat -ano | findstr :45678
   ```
2. Change the port in `config.env`:
   ```ini
   HTTP_PORT=45679
   ```
3. Ensure no firewall is blocking the port (for localhost, this is rare)

### Connection Refused

**Problem:** `curl: (7) Failed to connect to localhost port 45678`

**Solutions:**
1. Verify the GUI application is running
2. Check the status bar shows "HTTP API listening..."
3. Verify `API_ENABLED=true` in `config.env`
4. Restart the application after changing configuration

### NvAPI Not Initialized

**Problem:** All requests return `"NvAPI not initialized"`

**Solutions:**
1. Ensure you have an NVidia GPU with recent drivers
2. Verify a monitor is connected to the NVidia GPU (not integrated graphics)
3. Check the GUI for error messages about NVidia API initialization
4. Try restarting the application

### Commands Not Working on Monitor

**Problem:** API returns success but monitor doesn't change

**Solutions:**
1. Not all monitors support VCP commands (brightness/contrast)
2. Input switching uses LG Ultragear-specific commands - may not work on other brands
3. Try the GUI controls first to verify hardware compatibility
4. Some monitors require specific I2C commands - see main README for details

### Slow Response Times

**Problem:** API requests take several seconds

**Explanation:** This is normal. I2C monitor communication is slow (50-200ms per command). The HTTP server itself responds quickly, but the actual monitor control operation takes time.

---

## Limitations

1. **No SSL/TLS**: The API does not support HTTPS. Use localhost-only or implement a reverse proxy for remote access.
2. **No Authentication**: No built-in authentication mechanism. Relies on localhost-only binding for security.
3. **No Rate Limiting**: No protection against rapid repeated requests (though I2C operations are naturally slow).
4. **No WebSocket Support**: Real-time updates not available. Use polling with `/api/status` if needed.
5. **LG-Specific Input Switching**: Input source commands are designed for LG Ultragear monitors and may not work with other brands.
6. **Single Monitor Control**: Currently controls only the selected display in the GUI. Multi-monitor API support not yet implemented.
7. **No Read-Back Verification**: Commands are sent but monitor acknowledgment is not verified.

---

## Future Enhancements

Potential features for future versions:

- [ ] WebSocket support for real-time status updates
- [ ] Authentication (API key, OAuth)
- [ ] HTTPS/TLS support
- [ ] Multi-monitor API support (specify display index in request)
- [ ] Preset save/load endpoints
- [ ] Batch command endpoint (set multiple values in one request)
- [ ] Monitor capabilities detection
- [ ] Swagger/OpenAPI specification
- [ ] Rate limiting and request throttling

---

## API Version

Current API Version: **1.0.0**

The API follows semantic versioning. Breaking changes will increment the major version number.

---

## Support

For issues, questions, or feature requests:
1. Check the main README.md for general information
2. Review the troubleshooting section above
3. Open an issue on the project's GitHub repository

---

## License

This API is part of the Monitor Control application and inherits the same license as the main project.
