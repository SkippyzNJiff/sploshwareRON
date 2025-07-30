#pragma once
#include "imgui/imgui.h"
#include "ESP.h"
#include <string>

class Menu {
private:
    bool showMenu = true;
    bool showESPWindow = true;
    bool showAboutWindow = false;
    bool showStyleEditor = false;
    
    // Menu state
    int selectedTab = 0;
    
    // Style settings
    int currentStyle = 0;
    float menuAlpha = 0.95f;
    
    // Helper functions
    void ApplyStyle(int styleIndex);
    void RenderMainMenuBar();
    void RenderESPTab();
    void RenderVisualsTab();
    void RenderMiscTab();
    void RenderAboutWindow();
    void RenderStyleEditor();
    
public:
    Menu();
    ~Menu();
    
    void Render();
    void Toggle() { showMenu = !showMenu; }
    bool IsVisible() const { return showMenu; }
    
    // Hotkey handling
    void HandleInput();
};

extern Menu* g_Menu;