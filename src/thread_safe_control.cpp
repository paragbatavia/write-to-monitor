#include "thread_safe_control.h"
#include "monitor_control.h"
#include <stdio.h>

// AppState definition (must match monitor_control_gui.cpp)
struct AppState {
    float brightness = 50.0f;
    float contrast = 50.0f;
    int selected_display = 0;
    int display_count = 0;
    NvDisplayHandle displays[NVAPI_MAX_PHYSICAL_GPUS * NVAPI_MAX_DISPLAY_HEADS] = { 0 };
    NvPhysicalGpuHandle current_gpu = nullptr;
    NvU32 current_output_id = 0;
    bool nvapi_initialized = false;
    char status_message[256] = "Ready";
};

// LG Ultragear input source mappings
const InputSourceMapping ThreadSafeMonitorControl::input_mappings[4] = {
    {1, "HDMI 1",      0x90, 0xF4, 0x50},
    {2, "HDMI 2",      0x91, 0xF4, 0x50},
    {3, "DisplayPort", 0xD0, 0xF4, 0x50},
    {4, "USB-C",       0xD1, 0xF4, 0x50}
};

ThreadSafeMonitorControl::ThreadSafeMonitorControl(AppState* state)
    : app_state(state) {
}

bool ThreadSafeMonitorControl::SetBrightness(float brightness) {
    if (brightness < 0.0f || brightness > 100.0f) {
        return false;
    }

    std::lock_guard<std::mutex> lock(state_mutex);

    if (!app_state->nvapi_initialized) {
        return false;
    }

    BYTE value = (BYTE)brightness;
    BOOL result = WriteValueToMonitor(app_state->current_gpu, app_state->current_output_id,
                                     value, 0x10, 0x51); // 0x10 = brightness VCP code

    if (result) {
        app_state->brightness = brightness;
        snprintf(app_state->status_message, sizeof(app_state->status_message),
                "Brightness set to %.0f%% via API", brightness);
        return true;
    } else {
        snprintf(app_state->status_message, sizeof(app_state->status_message),
                "Failed to set brightness via API");
        return false;
    }
}

bool ThreadSafeMonitorControl::SetContrast(float contrast) {
    if (contrast < 0.0f || contrast > 100.0f) {
        return false;
    }

    std::lock_guard<std::mutex> lock(state_mutex);

    if (!app_state->nvapi_initialized) {
        return false;
    }

    BYTE value = (BYTE)contrast;
    BOOL result = WriteValueToMonitor(app_state->current_gpu, app_state->current_output_id,
                                     value, 0x12, 0x51); // 0x12 = contrast VCP code

    if (result) {
        app_state->contrast = contrast;
        snprintf(app_state->status_message, sizeof(app_state->status_message),
                "Contrast set to %.0f%% via API", contrast);
        return true;
    } else {
        snprintf(app_state->status_message, sizeof(app_state->status_message),
                "Failed to set contrast via API");
        return false;
    }
}

bool ThreadSafeMonitorControl::SetInputSource(int source) {
    if (source < 1 || source > 4) {
        return false;
    }

    std::lock_guard<std::mutex> lock(state_mutex);

    if (!app_state->nvapi_initialized) {
        return false;
    }

    const InputSourceMapping& mapping = input_mappings[source - 1];

    BOOL result = WriteValueToMonitor(app_state->current_gpu, app_state->current_output_id,
                                     mapping.input_value, mapping.command_code,
                                     mapping.register_address);

    if (result) {
        snprintf(app_state->status_message, sizeof(app_state->status_message),
                "Input switched to %s via API", mapping.name);
        return true;
    } else {
        snprintf(app_state->status_message, sizeof(app_state->status_message),
                "Failed to switch to %s via API", mapping.name);
        return false;
    }
}

float ThreadSafeMonitorControl::GetBrightness() {
    std::lock_guard<std::mutex> lock(state_mutex);
    return app_state->brightness;
}

float ThreadSafeMonitorControl::GetContrast() {
    std::lock_guard<std::mutex> lock(state_mutex);
    return app_state->contrast;
}

int ThreadSafeMonitorControl::GetSelectedDisplay() {
    std::lock_guard<std::mutex> lock(state_mutex);
    return app_state->selected_display;
}

bool ThreadSafeMonitorControl::IsInitialized() {
    std::lock_guard<std::mutex> lock(state_mutex);
    return app_state->nvapi_initialized;
}

std::string ThreadSafeMonitorControl::GetStatusMessage() {
    std::lock_guard<std::mutex> lock(state_mutex);
    return std::string(app_state->status_message);
}
