#ifndef MONITOR_CONTROL_H
#define MONITOR_CONTROL_H

#include <windows.h>
#include "nvapi.h"

// Function declarations for monitor control functionality
BOOL WriteValueToMonitor(NvPhysicalGpuHandle hPhysicalGpu, NvU32 displayId, BYTE input_value, BYTE command_code, BYTE register_address);

// Helper function for I2C checksum calculation
void CalculateI2cChecksum(const NV_I2C_INFO& i2cInfo);

// Initialization and cleanup functions
bool InitializeNvidiaAPI();
void CleanupNvidiaAPI();

// Display enumeration functions
bool EnumerateDisplays(NvDisplayHandle* displays, int* count);
bool GetGpuFromDisplay(NvDisplayHandle display, NvPhysicalGpuHandle* gpu, NvU32* outputId);
bool SelectDisplay(int display_index);

#endif // MONITOR_CONTROL_H

