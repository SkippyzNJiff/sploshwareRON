#include "includes.h"
#include <iostream>
#include <io.h>
#include <fcntl.h>
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ESP and Menu classes now enabled

// Console setup function
void SetupConsole() {
    AllocConsole();
    
    // Redirect stdout, stdin, stderr to console
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
    freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
    
    // Make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
    // point to console as well
    std::ios::sync_with_stdio(true);
    
    // Set console title
    SetConsoleTitleA("Ready or Not ESP - Debug Console");
    
    std::cout << "=== Ready or Not ESP Debug Console ===" << std::endl;
    std::cout << "DLL Injected successfully!" << std::endl;
    
    // Initialize offset loader
    std::cout << "Initializing Offset Loader..." << std::endl;
    g_OffsetLoader = new OffsetLoader();
    if (g_OffsetLoader->LoadOffsets()) {
        std::cout << "Offsets loaded successfully!" << std::endl;
        g_OffsetLoader->PrintLoadedOffsets();
    } else {
        std::cout << "WARNING: Failed to load some offsets, using fallback values" << std::endl;
    }
    
    // Silence verbose output after initialisation; toggle with F9 while running.
    Logger::EnableDebugLog(false);
    
    std::cout << "Initializing DirectX hook..." << std::endl;
}

Present oPresent;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;

void InitImGui()
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(pDevice, pContext);
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

bool init = false;
HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (!init)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)& pDevice)))
		{
			pDevice->GetImmediateContext(&pContext);
			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			window = sd.OutputWindow;
			ID3D11Texture2D* pBackBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)& pBackBuffer);
			pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
			pBackBuffer->Release();
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
			InitImGui();
			
			std::cout << "DirectX 11 device initialized successfully!" << std::endl;
			std::cout << "Window handle: 0x" << std::hex << window << std::dec << std::endl;
			std::cout << "ImGui initialized successfully!" << std::endl;
			
					// Initialize ESP and Menu - RE-ENABLED AFTER SUCCESSFUL INJECTION
		std::cout << "[MAIN] Initializing ESP and Menu systems..." << std::endl;
		g_ESP = new ESP();
		if (g_ESP) {
			std::cout << "[MAIN] ESP initialized successfully at: 0x" << std::hex << (uintptr_t)g_ESP << std::dec << std::endl;
		} else {
			std::cout << "[MAIN] ERROR: Failed to initialize ESP!" << std::endl;
		}
		
		g_Menu = new Menu();
		if (g_Menu) {
			std::cout << "[MAIN] Menu initialized successfully at: 0x" << std::hex << (uintptr_t)g_Menu << std::dec << std::endl;
		} else {
			std::cout << "[MAIN] ERROR: Failed to initialize Menu!" << std::endl;
		}
		std::cout << "[MAIN] ESP and Menu initialization complete!" << std::endl;
			
			init = true;
			std::cout << "Hook initialization complete!" << std::endl;
		}

		else
			return oPresent(pSwapChain, SyncInterval, Flags);
	}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	
	// Toggle verbose console output with F9
	if (GetAsyncKeyState(VK_F9) & 1) {
		Logger::EnableDebugLog(!Logger::g_enabled);
	}
	
	// ESP and Menu rendering with comprehensive logging
	static int frameCount = 0;
	frameCount++;
	
	if (frameCount % 120 == 0) { // Log every 2 seconds
		std::cout << "[MAIN] Frame " << frameCount << " - ESP: " << (g_ESP ? "OK" : "NULL") << ", Menu: " << (g_Menu ? "OK" : "NULL") << std::endl;
	}
	
	// Update and render ESP
	if (g_ESP) {
		g_ESP->Update();
		g_ESP->Render();
	} else {
		if (frameCount % 300 == 0) {
			std::cout << "[MAIN] WARNING: g_ESP is NULL!" << std::endl;
		}
	}
	
	// Render menu (toggle with INSERT key)
	if (g_Menu) {
		g_Menu->Render();
	} else {
		if (frameCount % 300 == 0) {
			std::cout << "[MAIN] WARNING: g_Menu is NULL!" << std::endl;
		}
	}
	
	// Simple status window for debugging
	static bool showStatus = true;
	if (showStatus) {
		ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(300, 150), ImGuiCond_FirstUseEver);
		
		if (ImGui::Begin("RON ESP Status", &showStatus, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Ready or Not ESP - ACTIVE");
			ImGui::Separator();
			
			if (g_ESP) {
				ImGui::Text("ESP Status: %s", g_ESP->GetSettings().enabled ? "ENABLED" : "DISABLED");
				ImGui::Text("Entities Found: %d", (int)g_ESP->GetEntityCount());
			}
			
			ImGui::Separator();
			ImGui::Text("Controls:");
			ImGui::Text("INSERT - Toggle Menu");
			
			if (ImGui::Button("Close")) {
				showStatus = false;
			}
		}
		ImGui::End();
	}

	ImGui::Render();

	pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	return oPresent(pSwapChain, SyncInterval, Flags);
}

DWORD WINAPI MainThread(LPVOID lpReserved)
{
	std::cout << "MainThread started..." << std::endl;
	
	bool init_hook = false;
	int attempts = 0;
	do
	{
		attempts++;
		std::cout << "Hook attempt #" << attempts << std::endl;
		
		if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
		{
			std::cout << "Kiero initialized successfully!" << std::endl;
			std::cout << "Binding Present function..." << std::endl;
			
			if (kiero::bind(8, (void**)& oPresent, hkPresent) == kiero::Status::Success) {
				std::cout << "Present function hooked successfully!" << std::endl;
				init_hook = true;
			} else {
				std::cout << "Failed to bind Present function!" << std::endl;
			}
		} else {
			std::cout << "Kiero initialization failed, retrying..." << std::endl;
			Sleep(1000); // Wait 1 second before retry
		}
		
		if (attempts > 10) {
			std::cout << "Failed to initialize hook after 10 attempts!" << std::endl;
			break;
		}
	} while (!init_hook);
	
	if (init_hook) {
		std::cout << "Hook setup complete! ESP should be active now." << std::endl;
	}
	
	return TRUE;
}

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hMod);
		
		// Setup debug console
		SetupConsole();
		
		CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);
		break;
	case DLL_PROCESS_DETACH:
		std::cout << "DLL being unloaded..." << std::endl;
		
		// ESP and Menu cleanup - RE-ENABLED
		if (g_ESP) {
			delete g_ESP;
			g_ESP = nullptr;
		}
		if (g_Menu) {
			delete g_Menu;
			g_Menu = nullptr;
		}
		
		std::cout << "DLL cleanup: ESP and Menu cleaned up" << std::endl;
		
		kiero::shutdown();
		FreeConsole();
		break;
	}
	return TRUE;
}