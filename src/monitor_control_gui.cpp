// Monitor Control GUI - ImGui Application
// Library linking is now handled by CMake - see CMakeLists.txt

#include <stdio.h>
#include <windows.h>
#include <d3d11.h>
#include <tchar.h>

// ImGui includes
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

// NVidia API
#include "nvapi.h"

// Monitor control functions
#include "monitor_control.h"

// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// GUI-specific functions
bool InitializeGUI();
bool SelectGUIDisplay(int display_index);

// Application state
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

static AppState g_app_state;

// GUI-specific initialization wrapper
bool InitializeGUI()
{
    NvAPI_Status status = NvAPI_Initialize();
    if (status != NVAPI_OK) {
        snprintf(g_app_state.status_message, sizeof(g_app_state.status_message), 
                "NvAPI_Initialize failed: %d", status);
        return false;
    }

    // Enumerate displays
    g_app_state.display_count = 0;
    for (unsigned int i = 0; status == NVAPI_OK && i < NVAPI_MAX_PHYSICAL_GPUS * NVAPI_MAX_DISPLAY_HEADS; i++) {
        status = NvAPI_EnumNvidiaDisplayHandle(i, &g_app_state.displays[i]);
        if (status == NVAPI_OK) {
            g_app_state.display_count++;
        } else if (status != NVAPI_END_ENUMERATION) {
            snprintf(g_app_state.status_message, sizeof(g_app_state.status_message),
                    "Display enumeration failed: %d", status);
            return false;
        }
    }

    if (g_app_state.display_count == 0) {
        snprintf(g_app_state.status_message, sizeof(g_app_state.status_message),
                "No NVidia displays found");
        return false;
    }

    // Initialize with first display
    return SelectGUIDisplay(0);
}

bool SelectGUIDisplay(int display_index)
{
    if (display_index < 0 || display_index >= g_app_state.display_count) {
        return false;
    }

    NvAPI_Status status;
    NvU32 gpu_count = 0;

    // Get GPU associated with display
    status = NvAPI_GetPhysicalGPUsFromDisplay(g_app_state.displays[display_index], 
                                            &g_app_state.current_gpu, &gpu_count);
    if (status != NVAPI_OK) {
        snprintf(g_app_state.status_message, sizeof(g_app_state.status_message),
                "Failed to get GPU for display %d: %d", display_index, status);
        return false;
    }

    // Get output ID
    status = NvAPI_GetAssociatedDisplayOutputId(g_app_state.displays[display_index], 
                                              &g_app_state.current_output_id);
    if (status != NVAPI_OK) {
        snprintf(g_app_state.status_message, sizeof(g_app_state.status_message),
                "Failed to get output ID for display %d: %d", display_index, status);
        return false;
    }

    g_app_state.selected_display = display_index;
    snprintf(g_app_state.status_message, sizeof(g_app_state.status_message),
            "Selected display %d", display_index);
    return true;
}

void SetBrightness(float brightness)
{
    if (!g_app_state.nvapi_initialized) return;

    BYTE value = (BYTE)(brightness * 100.0f / 100.0f); // scale to 0-100
    BOOL result = WriteValueToMonitor(g_app_state.current_gpu, g_app_state.current_output_id, 
                                    value, 0x10, 0x51); // 0x10 = brightness VCP code
    
    if (result) {
        g_app_state.brightness = brightness;
        snprintf(g_app_state.status_message, sizeof(g_app_state.status_message),
                "Brightness set to %.0f%%", brightness);
    } else {
        snprintf(g_app_state.status_message, sizeof(g_app_state.status_message),
                "Failed to set brightness");
    }
}

void SetContrast(float contrast)
{
    if (!g_app_state.nvapi_initialized) return;

    BYTE value = (BYTE)(contrast * 100.0f / 100.0f);
    BOOL result = WriteValueToMonitor(g_app_state.current_gpu, g_app_state.current_output_id, 
                                    value, 0x12, 0x51); // 0x12 = contrast VCP code
    
    if (result) {
        g_app_state.contrast = contrast;
        snprintf(g_app_state.status_message, sizeof(g_app_state.status_message),
                "Contrast set to %.0f%%", contrast);
    } else {
        snprintf(g_app_state.status_message, sizeof(g_app_state.status_message),
                "Failed to set contrast");
    }
}

// Main code
int main(int, char**)
{
    // Create application window
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Monitor Control", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Monitor Control - NVidia API", WS_OVERLAPPEDWINDOW, 100, 100, 800, 600, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Initialize NVidia API
    g_app_state.nvapi_initialized = InitializeGUI();

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Main window
        ImGui::Begin("Monitor Control");

        // Display selection
        if (g_app_state.nvapi_initialized && g_app_state.display_count > 1) {
            ImGui::Text("Display:");
            ImGui::SameLine();
            if (ImGui::Combo("##display", &g_app_state.selected_display, 
                           [](void* data, int idx, const char** out_text) {
                               static char buf[32];
                               snprintf(buf, sizeof(buf), "Display %d", idx);
                               *out_text = buf;
                               return true;
                           }, nullptr, g_app_state.display_count)) {
                SelectGUIDisplay(g_app_state.selected_display);
            }
            ImGui::Separator();
        }

        if (g_app_state.nvapi_initialized) {
            // Brightness control
            ImGui::Text("Brightness:");
            static float brightness_temp = g_app_state.brightness;
            if (ImGui::SliderFloat("##brightness", &brightness_temp, 0.0f, 100.0f, "%.0f%%")) {
                SetBrightness(brightness_temp);
            }

            ImGui::Spacing();

            // Contrast control
            ImGui::Text("Contrast:");
            static float contrast_temp = g_app_state.contrast;
            if (ImGui::SliderFloat("##contrast", &contrast_temp, 0.0f, 100.0f, "%.0f%%")) {
                SetContrast(contrast_temp);
            }

            ImGui::Separator();

            // Quick presets
            ImGui::Text("Quick Presets:");
            if (ImGui::Button("Bright")) {
                SetBrightness(80.0f);
                SetContrast(75.0f);
                brightness_temp = 80.0f;
                contrast_temp = 75.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Normal")) {
                SetBrightness(50.0f);
                SetContrast(50.0f);
                brightness_temp = 50.0f;
                contrast_temp = 50.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Dark")) {
                SetBrightness(20.0f);
                SetContrast(40.0f);
                brightness_temp = 20.0f;
                contrast_temp = 40.0f;
            }
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "NVidia API not initialized!");
            ImGui::Text("Make sure you have:");
            ImGui::BulletText("An NVidia GPU");
            ImGui::BulletText("Recent NVidia drivers");
            ImGui::BulletText("A monitor connected to the NVidia GPU");
        }

        ImGui::Separator();
        ImGui::Text("Status: %s", g_app_state.status_message);

        ImGui::End();

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions
bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
