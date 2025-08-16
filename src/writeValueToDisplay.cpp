// Library linking is now handled by CMake - see CMakeLists.txt

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <tchar.h>
#include "nvapi.h"
#include "monitor_control.h"


int main(int argc, char* argv[]) {

    int display_index = 0;
    BYTE input_value = 0;
    BYTE command_code = 0;  //VCP code or equivalent
    BYTE register_address = 0x51;

    // Usage: writeValueToMonitor.exe [display_index] [input_value] [command_code]
    // Uses default register addres 0x51 used for VCP codes
    if (argc == 4) {
        display_index = atoi(argv[1]);
        input_value = (BYTE)strtol(argv[2], NULL, 16);
        command_code = (BYTE)strtol(argv[3], NULL, 16);
    }

    // Usage: writeValueToMonitor.exe [display_index] [input_value] [command_code] [register_address]
    // Uses default register addres 0x51 used for VCP codes
    else if (argc == 5) {
        display_index = atoi(argv[1]);
        input_value = (BYTE)strtol(argv[2], NULL, 16);
        command_code = (BYTE)strtol(argv[3], NULL, 16);
        register_address = (BYTE)strtol(argv[4], NULL, 16);
    }
    else {
        printf("Incorrect Number of arguments!\n\n");

        printf("Arguments:\n");
        printf("display_index   - Index assigned to monitor (0 for first screen)\n");
        printf("input_value     - value to right to screen\n");
        printf("command_code    - VCP code or other\n");
        printf("register_address - Adress to write to, default 0x51 for VCP codes\n\n");

        printf("Usage:\n");
        printf("writeValueToScreen.exe [display_index] [input_value] [command_code]\n");
        printf("OR\n");
        printf("writeValueToScreen.exe [display_index] [input_value] [command_code] [register_address]\n");
        return 1;
    }
    

    //printf("%d, %ld, %d, %d", display_index, input_value, command_code, register_address);

    NvAPI_Status nvapiStatus = NVAPI_OK;

    // Initialize NVAPI.
    if ((nvapiStatus = NvAPI_Initialize()) != NVAPI_OK)
    {
        printf("NvAPI_Initialize() failed with status %d\n", nvapiStatus);
        return 1;
    }


    //
    // Enumerate display handles
    //
    NvDisplayHandle hDisplay_a[NVAPI_MAX_PHYSICAL_GPUS * NVAPI_MAX_DISPLAY_HEADS] = { 0 };
    for (unsigned int i = 0; nvapiStatus == NVAPI_OK; i++)
    {
        nvapiStatus = NvAPI_EnumNvidiaDisplayHandle(i, &hDisplay_a[i]);
        
        if (nvapiStatus != NVAPI_OK && nvapiStatus != NVAPI_END_ENUMERATION)
        {
            printf("NvAPI_EnumNvidiaDisplayHandle() failed with status %d\n", nvapiStatus);
            return 1;
        }
    }

   
    // Get GPU id assiciated with display ID
    NvPhysicalGpuHandle hGpu = NULL;
    NvU32 pGpuCount = 0;
    nvapiStatus = NvAPI_GetPhysicalGPUsFromDisplay(hDisplay_a[display_index], &hGpu, &pGpuCount);
    if (nvapiStatus != NVAPI_OK)
    {
        printf("NvAPI_GetPhysicalGPUFromDisplay() failed with status %d\n", nvapiStatus);
        return 1;
    }

    // Get the display id for subsequent I2C calls via NVAPI:
    NvU32 outputID = 0;
    nvapiStatus = NvAPI_GetAssociatedDisplayOutputId(hDisplay_a[display_index], &outputID);
    if (nvapiStatus != NVAPI_OK)
    {
        printf("NvAPI_GetAssociatedDisplayOutputId() failed with status %d\n", nvapiStatus);
        return 1;
    }


    BOOL result = WriteValueToMonitor(hGpu, outputID, input_value, command_code, register_address);
    if (!result)
    {
        printf("Changing input failed\n");
    }
    printf("\n");


}


