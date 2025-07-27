#include "Menu.h"
#include "ESP.h"
#include <Windows.h>
#include <iostream>

// Global Menu instance - re-enabled for linking
Menu* g_Menu = nullptr;

Menu::Menu() {
    std::cout << "[MENU] Initializing Menu system..." << std::endl;
    ApplyStyle(0); // Apply default dark style
    std::cout << "[MENU] Menu system initialized successfully!" << std::endl;
}

Menu::~Menu() {
}

void Menu::ApplyStyle(int styleIndex) {
    ImGuiStyle& style = ImGui::GetStyle();
    
    switch (styleIndex) {
        case 0: // Dark Theme
            ImGui::StyleColorsDark();
            style.WindowRounding = 6.0f;
            style.FrameRounding = 4.0f;
            style.PopupRounding = 4.0f;
            style.ScrollbarRounding = 4.0f;
            style.GrabRounding = 4.0f;
            style.TabRounding = 4.0f;
            break;
            
        case 1: // Light Theme
            ImGui::StyleColorsLight();
            style.WindowRounding = 6.0f;
            style.FrameRounding = 4.0f;
            style.PopupRounding = 4.0f;
            style.ScrollbarRounding = 4.0f;
            style.GrabRounding = 4.0f;
            style.TabRounding = 4.0f;
            break;
            
        case 2: // Classic Theme
            ImGui::StyleColorsClassic();
            break;
            
        case 3: // Custom Red Theme
            ImGui::StyleColorsDark();
            style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
            style.Colors[ImGuiCol_CheckMark] = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
            style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
            style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
            style.Colors[ImGuiCol_Button] = ImVec4(0.6f, 0.15f, 0.15f, 0.8f);
            style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
            style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
            break;
    }
    
    style.Alpha = menuAlpha;
}

void Menu::HandleInput() {
    // Toggle menu with INSERT key
    static bool insertPressed = false;
    bool insertDown = GetAsyncKeyState(VK_INSERT) & 0x8000;
    
    if (insertDown && !insertPressed) {
        Toggle();
    }
    insertPressed = insertDown;
}

void Menu::RenderMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Ready or Not ESP")) {
            if (ImGui::MenuItem("ESP Settings", nullptr, showESPWindow)) {
                showESPWindow = !showESPWindow;
            }
            if (ImGui::MenuItem("Style Editor", nullptr, showStyleEditor)) {
                showStyleEditor = !showStyleEditor;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("About", nullptr, showAboutWindow)) {
                showAboutWindow = !showAboutWindow;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Hide Menu", "INSERT")) {
                showMenu = false;
            }
            ImGui::EndMenu();
        }
        
        // Status indicators
        ImGui::Separator();
        if (g_ESP && g_ESP->GetSettings().enabled) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "ESP: ON");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "ESP: OFF");
        }
        
        ImGui::EndMainMenuBar();
    }
}

void Menu::RenderESPTab() {
    if (!g_ESP) return;
    
    ESPSettings& settings = g_ESP->GetSettings();
    
    ImGui::Text("ESP Configuration");
    ImGui::Separator();
    
    // Main ESP toggle
    if (ImGui::Checkbox("Enable ESP", &settings.enabled)) {
        // ESP toggled
    }
    
    ImGui::Spacing();
    
    // Entity types
    ImGui::Text("Entity Types:");
    ImGui::Indent();
    ImGui::Checkbox("Players", &settings.showPlayers);
    ImGui::SameLine();
    ImGui::ColorEdit4("##PlayerColor", (float*)&settings.playerColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    
    ImGui::Checkbox("NPCs/AI", &settings.showNPCs);
    ImGui::SameLine();
    ImGui::ColorEdit4("##NPCColor", (float*)&settings.npcColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    
    ImGui::Checkbox("Items", &settings.showItems);
    ImGui::SameLine();
    ImGui::ColorEdit4("##ItemColor", (float*)&settings.itemColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    ImGui::Unindent();
    
    ImGui::Spacing();
    
    // Display options
    ImGui::Text("Display Options:");
    ImGui::Indent();
    ImGui::Checkbox("Show Names", &settings.showNames);
    ImGui::Checkbox("Show Distance", &settings.showDistance);
    ImGui::SliderFloat("Max Distance", &settings.maxDistance, 50.0f, 1000.0f, "%.0fm");
    ImGui::Unindent();
    
    ImGui::Spacing();
    
    // Text color
    ImGui::Text("Text Settings:");
    ImGui::Indent();
    ImGui::ColorEdit4("Text Color", (float*)&settings.textColor);
    ImGui::Unindent();
}

void Menu::RenderVisualsTab() {
    ImGui::Text("Visual Settings");
    ImGui::Separator();
    
    // Style selection
    ImGui::Text("Theme:");
    const char* styles[] = { "Dark", "Light", "Classic", "Red" };
    if (ImGui::Combo("Style", &currentStyle, styles, IM_ARRAYSIZE(styles))) {
        ApplyStyle(currentStyle);
    }
    
    // Menu transparency
    if (ImGui::SliderFloat("Menu Alpha", &menuAlpha, 0.3f, 1.0f, "%.2f")) {
        ApplyStyle(currentStyle);
    }
    
    ImGui::Spacing();
    
    // Quick style buttons
    ImGui::Text("Quick Styles:");
    if (ImGui::Button("Dark##style")) {
        currentStyle = 0;
        ApplyStyle(currentStyle);
    }
    ImGui::SameLine();
    if (ImGui::Button("Light##style")) {
        currentStyle = 1;
        ApplyStyle(currentStyle);
    }
    ImGui::SameLine();
    if (ImGui::Button("Red##style")) {
        currentStyle = 3;
        ApplyStyle(currentStyle);
    }
}

void Menu::RenderMiscTab() {
    ImGui::Text("Miscellaneous");
    ImGui::Separator();
    
    // Debug info
    if (g_ESP) {
        ImGui::Text("ESP Status: %s", g_ESP->GetSettings().enabled ? "Enabled" : "Disabled");
        ImGui::Text("Entities Found: %d", (int)g_ESP->GetEntityCount());
    }
    
    ImGui::Spacing();
    
    // Controls info
    ImGui::Text("Controls:");
    ImGui::BulletText("INSERT - Toggle Menu");
    ImGui::BulletText("Use mouse to interact with menu");
    
    ImGui::Spacing();
    
    // Game info
    ImGui::Text("Game Information:");
    ImGui::BulletText("Game: Ready or Not");
    ImGui::BulletText("Mode: Offline Only");
    ImGui::BulletText("Graphics: DirectX 11");
    
    ImGui::Spacing();
    
    // Memory offsets info
    ImGui::Text("Memory Offsets:");
    ImGui::BulletText("GObjects: 0x8f86ef0");
    ImGui::BulletText("GWorld: 0x90F3A98");
    ImGui::BulletText("GNames: 0x8EE0B40");
}

void Menu::RenderAboutWindow() {
    if (!showAboutWindow) return;
    
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("About Ready or Not ESP", &showAboutWindow)) {
        ImGui::Text("Ready or Not ESP v1.0");
        ImGui::Separator();
        
        ImGui::Text("A simple ESP overlay for Ready or Not");
        ImGui::Text("Built with ImGui and DirectX 11 hooking");
        
        ImGui::Spacing();
        ImGui::Text("Features:");
        ImGui::BulletText("Player, NPC, and Item detection");
        ImGui::BulletText("Distance display");
        ImGui::BulletText("Customizable colors");
        ImGui::BulletText("Performance optimized");
        
        ImGui::Spacing();
        ImGui::Text("Usage:");
        ImGui::BulletText("Press INSERT to toggle menu");
        ImGui::BulletText("Configure ESP settings in the ESP tab");
        ImGui::BulletText("Works in offline mode only");
        
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Educational purposes only!");
        ImGui::TextWrapped("Use responsibly and respect game rules.");
        
        ImGui::Spacing();
        if (ImGui::Button("Close")) {
            showAboutWindow = false;
        }
    }
    ImGui::End();
}

void Menu::RenderStyleEditor() {
    if (!showStyleEditor) return;
    
    ImGui::Begin("Style Editor", &showStyleEditor);
    ImGui::ShowStyleEditor();
    ImGui::End();
}

void Menu::Render() {
    static int renderCount = 0;
    renderCount++;
    
    if (renderCount % 60 == 0) {
        std::cout << "[MENU] Render called - ShowMenu: " << (showMenu ? "true" : "false") << std::endl;
    }
    
    HandleInput();
    
    if (!showMenu) {
        if (renderCount % 120 == 0) {
            std::cout << "[MENU] Menu hidden, skipping render" << std::endl;
        }
        return;
    }
    
    std::cout << "[MENU] Rendering menu window..." << std::endl;
    
    // Render main menu bar
    RenderMainMenuBar();
    
    // Main menu window
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Ready or Not ESP Menu", &showMenu, ImGuiWindowFlags_MenuBar)) {
        
        // Tab bar
        if (ImGui::BeginTabBar("MainTabs")) {
            
            if (ImGui::BeginTabItem("ESP")) {
                RenderESPTab();
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Visuals")) {
                RenderVisualsTab();
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Misc")) {
                RenderMiscTab();
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
    
    // Render additional windows
    RenderAboutWindow();
    RenderStyleEditor();
}