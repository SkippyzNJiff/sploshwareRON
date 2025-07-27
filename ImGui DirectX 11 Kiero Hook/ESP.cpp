#include "ESP.h"
#include "GameSDK.h"
#include <algorithm>
#include <cmath>
#include <iostream>

// Global ESP instance - re-enabled for linking
ESP* g_ESP = nullptr;

ESP::ESP() {
    std::cout << "[ESP] Initializing ESP system..." << std::endl;
    UpdateScreenDimensions();
    std::cout << "[ESP] ESP system initialized successfully!" << std::endl;
}

ESP::~ESP() {
    entities.clear();
}

void ESP::UpdateScreenDimensions() {
    // Get screen dimensions from Windows
    screenWidth = (float)GetSystemMetrics(SM_CXSCREEN);
    screenHeight = (float)GetSystemMetrics(SM_CYSCREEN);
}

bool ESP::IsPlayerClass(const std::string& className) {
    // Ready or Not specific player classes
    return (className.find("BP_Player") != std::string::npos ||
            className.find("PlayerCharacter") != std::string::npos ||
            className.find("SWATCharacter") != std::string::npos ||
            className.find("Player") != std::string::npos ||
            className.find("FirstPersonCharacter") != std::string::npos ||
            className.find("BP_SWATCharacter") != std::string::npos);
}

bool ESP::IsNPCClass(const std::string& className) {
    // Ready or Not specific NPC/AI classes
    return (className.find("AI") != std::string::npos ||
            className.find("Bot") != std::string::npos ||
            className.find("Civilian") != std::string::npos ||
            className.find("Suspect") != std::string::npos ||
            className.find("Enemy") != std::string::npos ||
            className.find("NPC") != std::string::npos ||
            className.find("BP_AI") != std::string::npos ||
            className.find("BP_Civilian") != std::string::npos ||
            className.find("BP_Suspect") != std::string::npos ||
            className.find("CivilianCharacter") != std::string::npos ||
            className.find("SuspectCharacter") != std::string::npos ||
            className.find("AICharacter") != std::string::npos);
}

bool ESP::IsItemClass(const std::string& className) {
    // Ready or Not specific item/weapon classes
    return (className.find("Weapon") != std::string::npos ||
            className.find("Item") != std::string::npos ||
            className.find("Pickup") != std::string::npos ||
            className.find("Equipment") != std::string::npos ||
            className.find("Ammo") != std::string::npos ||
            className.find("BP_Weapon") != std::string::npos ||
            className.find("BP_Item") != std::string::npos ||
            className.find("BP_Equipment") != std::string::npos ||
            className.find("Rifle") != std::string::npos ||
            className.find("Pistol") != std::string::npos ||
            className.find("Shotgun") != std::string::npos ||
            className.find("Grenade") != std::string::npos ||
            className.find("Flashbang") != std::string::npos ||
            className.find("Evidence") != std::string::npos ||
            className.find("Door") != std::string::npos ||
            className.find("Interactable") != std::string::npos);
}

ImVec4 ESP::GetEntityColor(const ESPEntity& entity) {
    if (IsPlayerClass(entity.className)) {
        return settings.playerColor;
    } else if (IsNPCClass(entity.className)) {
        return settings.npcColor;
    } else if (IsItemClass(entity.className)) {
        return settings.itemColor;
    }
    return settings.textColor;
}

void ESP::Update() {
    static int updateCount = 0;
    updateCount++;
    
    if (updateCount % 60 == 0) { // Log every 60 updates to avoid spam
        std::cout << "[ESP] Update called (count: " << updateCount << ")" << std::endl;
    }
    
    entities.clear();
    
    if (!settings.enabled) {
        if (updateCount % 120 == 0) {
            std::cout << "[ESP] ESP disabled, skipping update" << std::endl;
        }
        return;
    }
    
    std::cout << "[ESP] ESP enabled, starting update..." << std::endl;
    
    try {
        // Get local player position for distance calculations
        std::cout << "[ESP] Getting camera location..." << std::endl;
        FVector localPlayerPos = GameSDK::GetCameraLocation();
        std::cout << "[ESP] Camera position: (" << localPlayerPos.X << ", " << localPlayerPos.Y << ", " << localPlayerPos.Z << ")" << std::endl;
        
        // Note: We now use a fallback position instead of (0,0,0), so we can continue
        // even if we can't get the exact player position
        
        // Get all actors in the world
        std::cout << "[ESP] Attempting to get all actors..." << std::endl;
        std::vector<AActor*> actors;
        
        try {
            actors = GameSDK::GetAllActors();
            std::cout << "[ESP] Found " << actors.size() << " actors to process" << std::endl;
            
            if (actors.empty()) {
                std::cout << "[ESP] No actors found, skipping update" << std::endl;
                return;
            }
        }
        catch (const std::exception& e) {
            std::cout << "[ESP] Exception in GetAllActors(): " << e.what() << " - Skipping actor processing." << std::endl;
            return; // Skip this update cycle
        }
        catch (...) {
            std::cout << "[ESP] Unknown exception in GetAllActors()! Skipping actor processing." << std::endl;
            return; // Skip this update cycle
        }
        
        for (AActor* actor : actors) {
            if (!actor || !GameSDK::IsValidPointer(actor)) continue;
            
            ESPEntity entity;
            entity.actor = actor;
            entity.worldPosition = actor->GetActorLocation();
            entity.name = actor->GetName();
            entity.className = entity.name; // Use name as className for now
            entity.distance = localPlayerPos.Distance(entity.worldPosition);
            
            // Skip if too close (likely the player themselves) or too far
            if (entity.distance < 5.0f || entity.distance > settings.maxDistance) continue;
            
            // Skip invalid positions
            if (entity.worldPosition.X == 0 && entity.worldPosition.Y == 0 && entity.worldPosition.Z == 0) continue;
            
            // Determine entity type and if it should be shown
            bool shouldShow = false;
            
            if (settings.showPlayers && IsPlayerClass(entity.className)) {
                shouldShow = true;
            } else if (settings.showNPCs && IsNPCClass(entity.className)) {
                shouldShow = true;
            } else if (settings.showItems && IsItemClass(entity.className)) {
                shouldShow = true;
            }
            
            // For Ready or Not, also check for common actor types
            if (!shouldShow) {
                // Check for Ready or Not specific classes
                if (entity.className.find("BP_") != std::string::npos ||
                    entity.className.find("Character") != std::string::npos ||
                    entity.className.find("Pawn") != std::string::npos ||
                    entity.className.find("AI") != std::string::npos ||
                    entity.className.find("Weapon") != std::string::npos ||
                    entity.className.find("Item") != std::string::npos) {
                    shouldShow = true;
                }
            }
            
            if (!shouldShow) continue;
            
            // Convert world position to screen position
            if (GameSDK::WorldToScreen(entity.worldPosition, entity.screenPosition)) {
                entity.isVisible = true;
                entity.color = GetEntityColor(entity);
                entities.push_back(entity);
            }
        }
        
        // Sort by distance (closest first)
        std::sort(entities.begin(), entities.end(), 
                 [](const ESPEntity& a, const ESPEntity& b) {
                     return a.distance < b.distance;
                 });
                 
        // Limit entities for performance (keep closest 100)
        if (entities.size() > 100) {
            entities.resize(100);
        }
    }
    catch (...) {
        // Handle any exceptions gracefully
        entities.clear();
    }
}

void ESP::Render() {
    static int renderCount = 0;
    renderCount++;
    
    if (renderCount % 60 == 0) { // Log every 60 renders
        std::cout << "[ESP] Render called (count: " << renderCount << ", enabled: " << (settings.enabled ? "true" : "false") << ")" << std::endl;
    }
    
    if (!settings.enabled) {
        if (renderCount % 120 == 0) {
            std::cout << "[ESP] ESP disabled, skipping render" << std::endl;
        }
        return;
    }
    
    std::cout << "[ESP] Rendering " << entities.size() << " ESP entities..." << std::endl;
    
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    
    for (const ESPEntity& entity : entities) {
        if (!entity.isVisible) continue;
        
        ImVec2 screenPos(entity.screenPosition.X, entity.screenPosition.Y);
        
        // Calculate size based on distance (closer = bigger)
        float baseSize = 6.0f;
        float distanceRatio = 100.0f / entity.distance;
        float sizeMultiplier = (distanceRatio < 0.5f) ? 0.5f : (distanceRatio > 2.0f) ? 2.0f : distanceRatio;
        float dotSize = baseSize * sizeMultiplier;
        
        // Draw entity indicator
        ImU32 colorU32 = ImGui::ColorConvertFloat4ToU32(entity.color);
        
        // Draw outer circle (border)
        drawList->AddCircleFilled(screenPos, dotSize + 1.0f, IM_COL32(0, 0, 0, 200));
        // Draw inner circle (main color)
        drawList->AddCircleFilled(screenPos, dotSize, colorU32);
        
        // Draw crosshair for better visibility
        float crossSize = dotSize + 3.0f;
        drawList->AddLine(
            ImVec2(screenPos.x - crossSize, screenPos.y),
            ImVec2(screenPos.x + crossSize, screenPos.y),
            colorU32, 1.0f
        );
        drawList->AddLine(
            ImVec2(screenPos.x, screenPos.y - crossSize),
            ImVec2(screenPos.x, screenPos.y + crossSize),
            colorU32, 1.0f
        );
        
        // Draw name and distance if enabled
        if (settings.showNames || settings.showDistance) {
            std::string displayText;
            
            if (settings.showNames && !entity.name.empty() && entity.name != "None") {
                displayText = entity.name;
                
                // Clean up the name for display
                if (displayText.length() > 30) {
                    displayText = displayText.substr(0, 27) + "...";
                }
            }
            
            if (settings.showDistance) {
                if (!displayText.empty()) displayText += " ";
                displayText += "[" + std::to_string((int)entity.distance) + "m]";
            }
            
            if (!displayText.empty()) {
                ImVec2 textSize = ImGui::CalcTextSize(displayText.c_str());
                ImVec2 textPos(screenPos.x - textSize.x * 0.5f, screenPos.y - textSize.y - 12.0f);
                
                // Ensure text stays on screen
                if (textPos.x < 0) textPos.x = 0;
                if (textPos.x + textSize.x > screenWidth) textPos.x = screenWidth - textSize.x;
                if (textPos.y < 0) textPos.y = screenPos.y + dotSize + 4.0f;
                
                // Draw text background with rounded corners
                drawList->AddRectFilled(
                    ImVec2(textPos.x - 3, textPos.y - 1),
                    ImVec2(textPos.x + textSize.x + 3, textPos.y + textSize.y + 1),
                    IM_COL32(0, 0, 0, 180),
                    2.0f
                );
                
                // Draw text with outline for better readability
                ImU32 textColorU32 = ImGui::ColorConvertFloat4ToU32(settings.textColor);
                
                // Text outline
                for (int dx = -1; dx <= 1; dx++) {
                    for (int dy = -1; dy <= 1; dy++) {
                        if (dx != 0 || dy != 0) {
                            drawList->AddText(
                                ImVec2(textPos.x + dx, textPos.y + dy),
                                IM_COL32(0, 0, 0, 255),
                                displayText.c_str()
                            );
                        }
                    }
                }
                
                // Main text
                drawList->AddText(textPos, textColorU32, displayText.c_str());
            }
        }
    }
}

// RenderSettings method removed - now handled by Menu class