#ifndef MONITOR_CONTROL_H
#define MONITOR_CONTROL_H

#include <windows.h>
#include "external/nvapi/nvapi.h"

// Function declarations for monitor control functionality
BOOL WriteValueToMonitor(NvPhysicalGpuHandle hPhysicalGpu, NvU32 displayId, BYTE input_value, BYTE command_code, BYTE register_address);

// Helper function for I2C checksum calculation
void CalculateI2cChecksum(const NV_I2C_INFO& i2cInfo);

// Initialization and cleanup functions
BOOL InitializeNvidiaAPI();
void CleanupNvidiaAPI();

// Display enumeration functions
BOOL EnumerateDisplays(NvDisplayHandle* displays, int* count);
BOOL GetGpuFromDisplay(NvDisplayHandle display, NvPhysicalGpuHandle* gpu, NvU32* outputId);

#endif // MONITOR_CONTROL_H

