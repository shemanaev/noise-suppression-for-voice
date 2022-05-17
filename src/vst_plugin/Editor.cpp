#include <iostream>

#include "Editor.h"
#include "RnNoiseVstPlugin.h"

// Data
static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

constexpr int WINDOW_WIDTH = 400;
constexpr int WINDOW_HEIGHT = 150;

Editor::Editor(AudioEffect* effect)
    : AEffEditor::AEffEditor(effect),
      m_rect{ 0, 0, WINDOW_HEIGHT, WINDOW_WIDTH }
{
}

Editor::~Editor()
{
    close();
}

bool Editor::getRect(ERect** rect)
{
    *rect = &m_rect;
    return true;
}

bool Editor::open(void* hostWindow)
{
	AEffEditor::open(hostWindow);

    if (m_running == true)
    {
        return false;
    }
    if (m_render_thread.joinable())
    {
        m_render_thread.join();
    }

    m_running = true;
    try
    {
        m_render_thread = std::thread(&Editor::render, this, (HWND)hostWindow);
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to start draw thread: " << e.what() << std::endl;
        return false;
    }

    return true;
}

void Editor::close()
{
	AEffEditor::close();

    m_running = false;

    if (m_render_thread.joinable())
    {
        m_render_thread.join();
    }
}

bool Editor::isOpen()
{
    return m_running;
}

void Editor::idle()
{
}

void Editor::render(HWND hostWindow)
{
    // Create application window
    ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEX wc = {
        sizeof(WNDCLASSEX), 
        CS_CLASSDC, 
        WndProc, 
        0L, 
        0L, 
        GetModuleHandle(NULL), 
        NULL, 
        NULL, 
        NULL, 
        NULL, 
        _T("Noise Suppression VST"), 
        NULL 
    };
    ::RegisterClassEx(&wc);
    m_window = ::CreateWindowEx(0, wc.lpszClassName, _T("Noise Suppression"), WS_CHILD, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, hostWindow, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(m_window))
    {
        CleanupDeviceD3D();
        ::DestroyWindow(m_window);
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        m_window = nullptr;
        return;
    }

    // Show the window
    ::ShowWindow(m_window, SW_SHOWDEFAULT);
    ::UpdateWindow(m_window);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(m_window);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    auto plugin = static_cast<RnNoiseVstPlugin*>(effect);

    while (m_running)
    {
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                m_running = false;
        }
        if (!m_running)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        {
            bool show_window = true;
            static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;

            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);

            ImGui::Begin("Settings", &show_window, flags);
            ImGui::PushItemWidth(WINDOW_WIDTH - 50);

            float vadThreshold = plugin->getVadThreshold();
            ImGui::Text("VAD Threshold");
            if (ImGui::SliderFloat("%", &vadThreshold, 0.0f, 1.0f)) {
                plugin->setVadThreshold(vadThreshold);
            }

            int vadRelease = plugin->getVadRelease();
            ImGui::Text("VAD Release");
            if (ImGui::SliderInt("ms", &vadRelease, 0, 1000)) {
                plugin->setVadRelease(vadRelease);
            }
            
            auto models = plugin->getAvailableModels();
            const char* current_item = plugin->getCurrentModel().c_str();
            ImGui::Text("Model");
            if (ImGui::BeginCombo("##combo", plugin->getCurrentModel().c_str()))
            {
                for (int n = 0; n < models.size(); n++)
                {
                    bool is_selected = (plugin->getCurrentModel().c_str() == models[n].c_str());
                    if (ImGui::Selectable(models[n].c_str(), is_selected)) {
                        plugin->setModel(models[n]);
                    }
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    if (hostWindow == NULL) {
        ::DestroyWindow(m_window);
    }
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    m_window = nullptr;
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
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    assert(pBackBuffer != nullptr);
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
