cmake_minimum_required(VERSION 3.16)
project(WriteToMonitor VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform-specific configurations
if(WIN32)
    # Windows-specific settings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    
    # Define architecture-specific library paths
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        # 64-bit
        set(NVAPI_LIB_PATH "${CMAKE_SOURCE_DIR}/external/nvapi/amd64/nvapi64.lib")
    else()
        # 32-bit
        set(NVAPI_LIB_PATH "${CMAKE_SOURCE_DIR}/external/nvapi/x86/nvapi.lib")
    endif()
else()
    message(FATAL_ERROR "This project currently only supports Windows due to NVidia API dependency")
endif()

# Check if NVidia API files exist
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/external/nvapi/nvapi.h")
    message(FATAL_ERROR "NVidia API not found. Please ensure the nvapi submodule is properly initialized.")
endif()

if(NOT EXISTS "${NVAPI_LIB_PATH}")
    message(FATAL_ERROR "NVidia API library not found at: ${NVAPI_LIB_PATH}")
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external/nvapi
)

# Create the main executable
add_executable(writeValueToDisplay
    src/writeValueToDisplay.cpp
)

# Link libraries
target_link_libraries(writeValueToDisplay
    ${NVAPI_LIB_PATH}
)

# Set output directory
set_target_properties(writeValueToDisplay PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Optional: Set up for future GUI development
# Uncomment these when adding GUI support:

# Qt option
# option(USE_QT "Build with Qt GUI" OFF)
# if(USE_QT)
#     find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
#     qt_standard_project_setup()
# endif()

# Dear ImGui option  
# option(USE_IMGUI "Build with Dear ImGui" OFF)
# if(USE_IMGUI)
#     # Add Dear ImGui setup here
# endif()

# Windows API GUI option
# option(USE_WIN32_GUI "Build with native Windows GUI" OFF)
# if(USE_WIN32_GUI)
#     target_link_libraries(writeValueToDisplay user32 gdi32 comctl32)
# endif()

# Install targets
install(TARGETS writeValueToDisplay
    RUNTIME DESTINATION bin
)

# CPack configuration for creating installers
set(CPACK_PACKAGE_NAME "WriteToMonitor")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Monitor Control via NVidia API")
set(CPACK_PACKAGE_VENDOR "Your Name")
include(CPack)
