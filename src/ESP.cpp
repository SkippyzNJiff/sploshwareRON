#include "ESP.h"
#include "GameSDK.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <cstdio>

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
    // More generic player detection for Ready or Not
    std::string lowerName = className;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    return (lowerName.find("player") != std::string::npos ||
            lowerName.find("character") != std::string::npos ||
            lowerName.find("swat") != std::string::npos ||
            lowerName.find("firstperson") != std::string::npos ||
            lowerName.find("pawn") != std::string::npos ||
            lowerName.find("bp_player") != std::string::npos ||
            lowerName.find("bp_character") != std::string::npos ||
            lowerName.find("bp_swat") != std::string::npos);
}

bool ESP::IsNPCClass(const std::string& className) {
    // More generic NPC/AI detection for Ready or Not
    std::string lowerName = className;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    return (lowerName.find("ai") != std::string::npos ||
            lowerName.find("bot") != std::string::npos ||
            lowerName.find("civilian") != std::string::npos ||
            lowerName.find("suspect") != std::string::npos ||
            lowerName.find("enemy") != std::string::npos ||
            lowerName.find("npc") != std::string::npos ||
            lowerName.find("bp_ai") != std::string::npos ||
            lowerName.find("bp_civilian") != std::string::npos ||
            lowerName.find("bp_suspect") != std::string::npos ||
            lowerName.find("hostage") != std::string::npos ||
            lowerName.find("criminal") != std::string::npos ||
            lowerName.find("terrorist") != std::string::npos);
}

bool ESP::IsItemClass(const std::string& className) {
    // More generic item detection for Ready or Not
    std::string lowerName = className;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    return (lowerName.find("weapon") != std::string::npos ||
            lowerName.find("item") != std::string::npos ||
            lowerName.find("pickup") != std::string::npos ||
            lowerName.find("equipment") != std::string::npos ||
            lowerName.find("ammo") != std::string::npos ||
            lowerName.find("rifle") != std::string::npos ||
            lowerName.find("pistol") != std::string::npos ||
            lowerName.find("shotgun") != std::string::npos ||
            lowerName.find("grenade") != std::string::npos ||
            lowerName.find("flashbang") != std::string::npos ||
            lowerName.find("evidence") != std::string::npos ||
            lowerName.find("door") != std::string::npos ||
            lowerName.find("interactable") != std::string::npos ||
            lowerName.find("collectible") != std::string::npos ||
            lowerName.find("gadget") != std::string::npos);
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
    // Run a full scan every frame. The old implementation throttled updates and
    // caused the ESP to appear frozen for several seconds on slower machines.
    // With the heavy debug logging removed this scan is cheap enough.
    bool shouldRefreshEntities = true;
    
    if (!settings.enabled) {
        return;
    }
    
    // Skip refresh on intermediate frames – keeps previously-collected list
    if (!shouldRefreshEntities) {
        return;
    }
    
    // ------------------------------------
    // Full scan path – executed every frame
    // ------------------------------------
    entities.clear();
    // Perform a complete actor scan
    
    try {
        // Get local player position for distance calculations
        FVector localPlayerPos = GameSDK::GetCameraLocation();
        
        // Get all actors in the world
        std::vector<AActor*> actors;
        
        try {
            actors = GameSDK::GetAllActors();
            if (actors.empty()) {
                return;
            }
        }
        catch (const std::exception& e) {
            return;
        }
        catch (...) {
            return;
        }
        
        int validActors = 0;
        int processedActors = 0;
        
        for (AActor* actor : actors) {
            processedActors++;
            
            if (!actor || !GameSDK::IsValidPointer(actor)) {
                continue;
            }
            
            try {
                ESPEntity entity;
                entity.actor = actor;
                entity.worldPosition = actor->GetActorLocation();
                
                // Skip actors with invalid positions
                if (entity.worldPosition.X == 0 && entity.worldPosition.Y == 0 && entity.worldPosition.Z == 0) {
                    continue;
                }
                
                entity.name = actor->GetName();
                entity.className = entity.name; // Use name as className for now

                // Skip known irrelevant engine/helper actors early (nav links, lights, etc.)
                static const std::vector<std::string> blacklist = {
                    "navlink", "navigation", "navmesh", "recast", "bp_nav", "navproxy",
                    "decal", "light", "sky", "fog", "postprocess", "defaultscene", "staticmesh",
                    "billboard", "arrowcomponent", "scenecomponent" };

                std::string lowerNameTmp = entity.name;
                std::transform(lowerNameTmp.begin(), lowerNameTmp.end(), lowerNameTmp.begin(), ::tolower);
                bool blacklisted = false;
                for (const auto& bad : blacklist) {
                    if (lowerNameTmp.find(bad) != std::string::npos) { blacklisted = true; break; }
                }
                if (blacklisted) {
                    continue; // skip this actor entirely
                }
                
                entity.distance = localPlayerPos.Distance(entity.worldPosition);
                
                // Skip if too close (likely the player themselves) or too far
                if (entity.distance < 10.0f || entity.distance > settings.maxDistance) {
                    continue;
                }
                
                // Determine entity type and if it should be shown
                bool shouldShow = false;
                
                // Try specific classification first
                if (settings.showPlayers && IsPlayerClass(entity.className)) {
                    shouldShow = true;
                    entity.color = settings.playerColor;
                } else if (settings.showNPCs && IsNPCClass(entity.className)) {
                    shouldShow = true;
                    entity.color = settings.npcColor;
                } else if (settings.showItems && IsItemClass(entity.className)) {
                    shouldShow = true;
                    entity.color = settings.itemColor;
                }
                
                // Optionally, include generic Blueprint actors if user enabled 'showItems'
                if (!shouldShow && settings.showItems && entity.className.find("BP_") != std::string::npos) {
                    shouldShow = true;
                    entity.color = ImVec4(1.0f, 0.5f, 0.0f, 1.0f); // Orange for generic Blueprint actors
                }
 
                if (!shouldShow) continue;
                
                // Convert world position to screen position
                if (GameSDK::WorldToScreen(entity.worldPosition, entity.screenPosition)) {
                    entity.isVisible = true;
                    entities.push_back(entity);
                    validActors++;
                }
            }
            catch (const std::exception& e) {
                // Skip this actor but continue with others
                continue;
            }
            catch (...) {
                // Skip this actor but continue with others
                continue;
            }
        }
        
        // processedActors and validActors are kept for potential debugging
        
        // Sort by distance (closest first)
        std::sort(entities.begin(), entities.end(), 
                 [](const ESPEntity& a, const ESPEntity& b) {
                     return a.distance < b.distance;
                 });
                 
        // Limit entities for performance (keep closest 50)
        if (entities.size() > 50) {
            entities.resize(50);
        }
    }
    catch (const std::exception& e) {
        entities.clear();
    }
    catch (...) {
        entities.clear();
    }
}

void ESP::Render() {
    static int renderCount = 0;
    renderCount++;
    
    if (!settings.enabled) {
        return;
    }
    
    // Use foreground draw list so ESP elements render above all other windows and the game scene
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    
    // Debug visualization - always show a test indicator
    
    // Draw a simple test indicator in the top-left corner to verify ESP is rendering
    ImVec2 testPos(50, 50);
    drawList->AddCircleFilled(testPos, 5.0f, IM_COL32(0, 255, 0, 255)); // Green dot
    drawList->AddText(ImVec2(70, 45), IM_COL32(255, 255, 255, 255), "ESP ACTIVE");
    
    // Show entity count
    std::string entityText = "Entities: " + std::to_string(entities.size());
    drawList->AddText(ImVec2(70, 65), IM_COL32(255, 255, 255, 255), entityText.c_str());

    // Draw FPS counter just below the entity count
    float fps = ImGui::GetIO().Framerate;
    char fpsText[32];
    sprintf_s(fpsText, "FPS: %.1f", fps);
    drawList->AddText(ImVec2(70, 85), IM_COL32(255, 255, 0, 255), fpsText);
    
    
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