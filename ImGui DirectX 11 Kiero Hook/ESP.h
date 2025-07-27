#pragma once
#include "GameSDK.h"
#include "imgui/imgui.h"
#include <vector>
#include <string>

struct ESPSettings {
    bool enabled = true;
    bool showPlayers = true;
    bool showNPCs = true;
    bool showItems = true;
    bool showDistance = true;
    bool showNames = true;
    float maxDistance = 500.0f;
    
    // Colors
    ImVec4 playerColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);  // Red
    ImVec4 npcColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);     // Yellow
    ImVec4 itemColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);    // Green
    ImVec4 textColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);    // White
};

struct ESPEntity {
    AActor* actor;
    FVector worldPosition;
    FVector2D screenPosition;
    std::string name;
    std::string className;
    float distance;
    bool isVisible;
    ImVec4 color;
};

class ESP {
private:
    ESPSettings settings;
    std::vector<ESPEntity> entities;
    
    // Screen dimensions
    float screenWidth = 1920.0f;
    float screenHeight = 1080.0f;
    
    // Helper functions
    bool IsPlayerClass(const std::string& className);
    bool IsNPCClass(const std::string& className);
    bool IsItemClass(const std::string& className);
    ImVec4 GetEntityColor(const ESPEntity& entity);
    void UpdateScreenDimensions();
    
public:
    ESP();
    ~ESP();
    
    void Update();
    void Render();
    
    ESPSettings& GetSettings() { return settings; }
    size_t GetEntityCount() const { return entities.size(); }
};

// Global ESP instance
extern ESP* g_ESP;