#ifndef THREAD_SAFE_CONTROL_H
#define THREAD_SAFE_CONTROL_H

#include <mutex>
#include <string>
#include <windows.h>
#include "nvapi.h"

// Forward declaration
struct AppState;

// Input source mapping for LG Ultragear monitors
struct InputSourceMapping {
    int api_value;          // 1-4 from API
    const char* name;       // Display name
    BYTE input_value;       // Value to send to monitor
    BYTE command_code;      // Command code (0xF4 for LG)
    BYTE register_address;  // Register address (0x50 for LG)
};

// Thread-safe wrapper for monitor control operations
class ThreadSafeMonitorControl {
private:
    std::mutex state_mutex;
    AppState* app_state;

    static const InputSourceMapping input_mappings[4];

public:
    ThreadSafeMonitorControl(AppState* state);

    // Thread-safe monitor control operations
    bool SetBrightness(float brightness);
    bool SetContrast(float contrast);
    bool SetInputSource(int source); // 1=HDMI 1, 2=HDMI 2, 3=DisplayPort, 4=USB-C

    // Thread-safe getters
    float GetBrightness();
    float GetContrast();
    int GetSelectedDisplay();
    bool IsInitialized();
    std::string GetStatusMessage();
};

#endif // THREAD_SAFE_CONTROL_H
