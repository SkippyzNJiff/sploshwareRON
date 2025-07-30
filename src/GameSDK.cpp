#include "GameSDK.h"
#include <iostream>
#include <cmath>
#include <cstdint>
#include <cwchar>

// UObject implementation - Updated for RON 5.3.2-0+Unknown SDK
// High-performance FName → std::string conversion using the engine's AppendString implementation
std::string UObject::GetName() {
    // Validate pointer quickly (avoid verbose logging for performance)
    if (!GameSDK::IsValidPointer(this)) {
        return "InvalidObject";
    }

    // Local helper representations matching the engine layout
    struct FNameTemp {
        uint32_t ComparisonIndex;
        uint32_t Number;
    };

    struct FStringTemp {
        wchar_t* Data;
        int32_t  Num;
        int32_t  Max;
    };

    uint64_t rawName = this->Name;
    if (rawName == 0) {
        return "None";
    }

    FNameTemp fname{};
    fname.ComparisonIndex = static_cast<uint32_t>(rawName & 0xFFFFFFFF);
    fname.Number          = static_cast<uint32_t>(rawName >> 32);

    // Resolve engine's AppendString function once
    static uintptr_t appendStringAddr = 0;
    if (!appendStringAddr) {
        uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
        uintptr_t offset     = g_OffsetLoader ? g_OffsetLoader->GetAppendStringOffset() : 0xC8DBB0;
        appendStringAddr     = moduleBase + offset;
    }

    if (!appendStringAddr) {
        return "None";
    }

    using AppendStringFn = void(*)(FNameTemp*, FStringTemp&);
    AppendStringFn AppendString = reinterpret_cast<AppendStringFn>(appendStringAddr);

    wchar_t buffer[256]{};
    FStringTemp temp{};
    temp.Data = buffer;
    temp.Max  = 256;
    temp.Num  = 0;

    // Call the game function to populate the buffer
    AppendString(&fname, temp);

    // Determine length (Num may not be set correctly in some builds)
    size_t wLen = (temp.Num > 0 && temp.Num < 256) ? static_cast<size_t>(temp.Num) : wcslen(buffer);

    std::wstring wName(buffer, wLen);
    std::string  result(wName.begin(), wName.end());

    // Strip package path (text before last '/') for readability
    size_t pos = result.rfind('/') ;
    if (pos != std::string::npos) {
        result = result.substr(pos + 1);
    }

    if (result.empty()) result = "None";
    return result;
}

bool UObject::IsA(const std::string& className) {
    try {
        UClass* objClass = GetClass();
        if (!objClass || !GameSDK::IsValidPointer(objClass)) return false;
        
        // Walk up the class hierarchy using UStruct inheritance
        // UClass inherits from UStruct which has Super at offset 0x40
        UClass* currentClass = objClass;
        for (int i = 0; i < 50 && currentClass; i++) {  // Prevent infinite loops
            if (!GameSDK::IsValidPointer(currentClass)) break;
            
            std::string currentClassName = currentClass->GetName();
            if (currentClassName.find(className) != std::string::npos) {
                return true;
            }
            
            // Get super class from UStruct structure (see CoreUObject_classes.hpp)
            currentClass = *(UClass**)((uintptr_t)currentClass + 0x40);
        }
        
        return false;
    }
    catch (...) {
        return false;
    }
}

// GameSDK namespace implementation
namespace GameSDK {
    bool IsValidPointer(void* ptr) {
#if ENABLE_GAMESDK_DEBUG
        std::cout << "[GameSDK] IsValidPointer() called with: 0x" << std::hex << (uintptr_t)ptr << std::dec << std::endl;
#endif

        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        if (addr < 0x10000 || addr > 0x7FFFFFFFFFFF) return false; // basic range check

        return true; // assume valid – avoids costly VirtualQuery per call
    }
    
    std::vector<AActor*> GetAllActorsFromGObjects() {
        std::vector<AActor*> actors;
        
        std::cout << "[GameSDK] GetAllActorsFromGObjects() - Fallback method" << std::endl;
        
        try {
            uintptr_t gObjects = UObject::GetGObjects();
            if (!gObjects || !IsValidPointer((void*)gObjects)) {
                std::cout << "[GameSDK] ERROR: Invalid GObjects pointer!" << std::endl;
                return actors;
            }
            
            std::cout << "[GameSDK] GObjects found at: 0x" << std::hex << gObjects << std::dec << std::endl;
            
            // Try to read GObjects array structure
            // TUObjectArray typically has: Objects (8 bytes), MaxElements (4 bytes), NumElements (4 bytes)
            uintptr_t objectArray = *(uintptr_t*)gObjects;
            int32_t numElements = *(int32_t*)(gObjects + 0xC);
            
            std::cout << "[GameSDK] GObjects array: 0x" << std::hex << objectArray << ", NumElements: " << std::dec << numElements << std::endl;
            
            if (!objectArray || !IsValidPointer((void*)objectArray) || numElements <= 0 || numElements > 500000) {
                std::cout << "[GameSDK] Invalid GObjects structure" << std::endl;
                return actors;
            }
            
            // Scan through objects looking for actors
            int validActors = 0;
            int maxScan = (numElements > 10000) ? 10000 : numElements; // Limit scan for performance
            
            for (int i = 0; i < maxScan; i++) {
                try {
                    uintptr_t objectPtr = objectArray + (i * 0x18); // FUObjectItem is typically 0x18 bytes
                    
                    if (!IsValidPointer((void*)objectPtr)) continue;
                    
                    UObject* obj = *(UObject**)objectPtr;
                    if (!obj || !IsValidPointer(obj)) continue;
                    
                    // Skip costly name lookup if GNames unavailable
                    std::string objName;
                    if (UObject::GetGNames()) {
                        objName = obj->GetName();
                    }

                    if (objName.empty()) objName = "Unknown";
                    
                    // Basic check for actor-like objects
                    if (objName.find("Actor") != std::string::npos ||
                        objName.find("BP_") != std::string::npos ||
                        objName.find("Character") != std::string::npos ||
                        objName.find("Pawn") != std::string::npos ||
                        objName.find("Player") != std::string::npos) {
                        
                        AActor* actor = (AActor*)obj;
                        actors.push_back(actor);
                        validActors++;
                        
                        if (validActors >= 100) break; // Limit for performance
                    }
                }
                catch (...) {
                    continue;
                }
            }
            
            std::cout << "[GameSDK] Found " << validActors << " actors through GObjects scan" << std::endl;
        }
        catch (const std::exception& e) {
            std::cout << "[GameSDK] Exception in GetAllActorsFromGObjects: " << e.what() << std::endl;
        }
        catch (...) {
            std::cout << "[GameSDK] Unknown exception in GetAllActorsFromGObjects" << std::endl;
        }
        
        return actors;
    }
    
    std::vector<AActor*> GetAllActors() {
        std::vector<AActor*> allActors;
        
        std::cout << "[GameSDK] GetAllActors() called" << std::endl;
        
        try {
            // Method 1: Try ULevel approach first
            std::cout << "[GameSDK] Attempting Method 1: ULevel approach..." << std::endl;
            UWorld* world = UWorld::GetWorld();
            if (world && IsValidPointer(world)) {
                ULevel* level = world->GetPersistentLevel();
                if (level && IsValidPointer(level)) {
                    std::vector<AActor*> actors = level->GetActors();
                    if (!actors.empty()) {
                        std::cout << "[GameSDK] ULevel method succeeded with " << actors.size() << " actors" << std::endl;
                        return actors;
                    }
                }
            }
            
            std::cout << "[GameSDK] ULevel method failed, trying fallback..." << std::endl;
            
            // Method 2: Fallback to GObjects scan
            std::cout << "[GameSDK] Attempting Method 2: GObjects scan..." << std::endl;
            allActors = GetAllActorsFromGObjects();
            
            if (!allActors.empty()) {
                std::cout << "[GameSDK] GObjects method succeeded with " << allActors.size() << " actors" << std::endl;
                return allActors;
            }
            
            std::cout << "[GameSDK] All methods failed to find actors" << std::endl;
        }
        catch (const std::exception& e) {
            std::cout << "[GameSDK] Exception in GetAllActors(): " << e.what() << std::endl;
        }
        catch (...) {
            std::cout << "[GameSDK] Unknown exception in GetAllActors()" << std::endl;
        }
        
        return allActors;
    }
    
    APlayerController* GetLocalPlayerController() {
        try {
            UWorld* world = UWorld::GetWorld();
            if (!world || !IsValidPointer(world)) {
                return nullptr;
            }
            
            APlayerController* controller = world->GetFirstPlayerController();
            return controller;
        }
        catch (...) {
            return nullptr;
        }
    }
    
    APawn* GetLocalPlayerPawn() {
        try {
            APlayerController* controller = GetLocalPlayerController();
            if (!controller || !IsValidPointer(controller)) {
                return nullptr;
            }
            
            APawn* pawn = controller->GetPawn();
            return pawn;
        }
        catch (...) {
            return nullptr;
        }
    }
    
    FVector GetCameraLocation() {
        std::cout << "[GameSDK] GetCameraLocation() called" << std::endl;
        
        try {
            std::cout << "[GameSDK] Getting UWorld..." << std::endl;
            UWorld* world = UWorld::GetWorld();
            if (!world) {
                std::cout << "[GameSDK] ERROR: UWorld::GetWorld() returned NULL!" << std::endl;
                return FVector(100.0f, 100.0f, 100.0f); // Return a non-zero fallback position
            }
            if (!IsValidPointer(world)) {
                std::cout << "[GameSDK] ERROR: UWorld pointer is invalid!" << std::endl;
                return FVector(100.0f, 100.0f, 100.0f); // Return a non-zero fallback position
            }
            std::cout << "[GameSDK] UWorld found at: 0x" << std::hex << (uintptr_t)world << std::dec << std::endl;
            
            std::cout << "[GameSDK] Getting local player pawn..." << std::endl;
            APawn* localPawn = GetLocalPlayerPawn();
            if (!localPawn) {
                std::cout << "[GameSDK] ERROR: GetLocalPlayerPawn() returned NULL!" << std::endl;
                
                // Fallback: Try to find player-like actors in the world
                std::cout << "[GameSDK] Attempting fallback: searching for player actors..." << std::endl;
                std::vector<AActor*> actors = GetAllActors();
                for (AActor* actor : actors) {
                    if (!actor || !IsValidPointer(actor)) continue;
                    
                    std::string actorName = actor->GetName();
                    if (actorName.find("Player") != std::string::npos ||
                        actorName.find("BP_Player") != std::string::npos ||
                        actorName.find("PlayerCharacter") != std::string::npos ||
                        actorName.find("FirstPerson") != std::string::npos) {
                        FVector actorLoc = actor->GetActorLocation();
                        if (actorLoc.X != 0 || actorLoc.Y != 0 || actorLoc.Z != 0) {
                            std::cout << "[GameSDK] Found potential player actor: " << actorName 
                                     << " at (" << actorLoc.X << ", " << actorLoc.Y << ", " << actorLoc.Z << ")" << std::endl;
                            return actorLoc;
                        }
                    }
                }
                
                return FVector(100.0f, 100.0f, 100.0f); // Return a non-zero fallback position
            }
            if (!IsValidPointer(localPawn)) {
                std::cout << "[GameSDK] ERROR: Local player pawn pointer is invalid!" << std::endl;
                return FVector(100.0f, 100.0f, 100.0f); // Return a non-zero fallback position
            }
            std::cout << "[GameSDK] Local player pawn found at: 0x" << std::hex << (uintptr_t)localPawn << std::dec << std::endl;
            
            std::cout << "[GameSDK] Getting actor location..." << std::endl;
            FVector location = localPawn->GetActorLocation();
            std::cout << "[GameSDK] Camera location: (" << location.X << ", " << location.Y << ", " << location.Z << ")" << std::endl;
            
            // If location is invalid, return a non-zero fallback
            if (location.X == 0 && location.Y == 0 && location.Z == 0) {
                std::cout << "[GameSDK] WARNING: Got zero location, returning fallback position" << std::endl;
                return FVector(100.0f, 100.0f, 100.0f);
            }
            
            return location;
        }
        catch (const std::exception& e) {
            std::cout << "[GameSDK] Exception in GetCameraLocation(): " << e.what() << std::endl;
            return FVector(100.0f, 100.0f, 100.0f); // Return a non-zero fallback position
        }
        catch (...) {
            std::cout << "[GameSDK] Unknown exception in GetCameraLocation()" << std::endl;
            return FVector(100.0f, 100.0f, 100.0f); // Return a non-zero fallback position
        }
    }
    
    FRotator GetCameraRotation() {
        try {
            APlayerController* controller = GetLocalPlayerController();
            if (!controller || !IsValidPointer(controller)) return FRotator();
            
            // Get PlayerCameraManager from controller
            uintptr_t cameraManager = *(uintptr_t*)((uintptr_t)controller + 0x348);
            if (!cameraManager || !IsValidPointer((void*)cameraManager)) {
                // Fallback: try to get rotation from controller directly
                // Control rotation is typically at offset 0x298
                return *(FRotator*)((uintptr_t)controller + 0x298);
            }
            
            // Get camera rotation from camera manager (typically at offset 0x1C0)
            return *(FRotator*)((uintptr_t)cameraManager + 0x1C0);
        }
        catch (...) {
            return FRotator();
        }
    }
    
    bool WorldToScreen(const FVector& worldPos, FVector2D& screenPos) {
        static int callCount = 0;
        callCount++;
        
        if (callCount % 30 == 0) { // Log every 30 calls to reduce spam
            std::cout << "[GameSDK] WorldToScreen called for position: (" << worldPos.X << ", " << worldPos.Y << ", " << worldPos.Z << ")" << std::endl;
        }
        
        try {
            // Get camera location first
            FVector cameraLocation = GetCameraLocation();
            
            // Use simplified projection method that's more reliable
            FVector direction = worldPos - cameraLocation;
            float distance = direction.Distance(FVector());
            
            if (distance < 1.0f) {
                return false; // Too close
            }
            
            // Get screen dimensions
            float screenWidth = (float)GetSystemMetrics(SM_CXSCREEN);
            float screenHeight = (float)GetSystemMetrics(SM_CYSCREEN);
            
            // Simple perspective projection
            // This is a simplified version that should work even if camera rotation is incorrect
            float projectionScale = 1000.0f; // Adjust this value to scale the projection
            
            // Project X and Z components to screen space
            // Using a simple orthographic-like projection for testing
            screenPos.X = (screenWidth * 0.5f) + (direction.X / distance) * projectionScale;
            screenPos.Y = (screenHeight * 0.5f) - (direction.Z / distance) * projectionScale;
            
            // Check if on screen
            bool onScreen = (screenPos.X >= -100 && screenPos.X <= screenWidth + 100 && 
                           screenPos.Y >= -100 && screenPos.Y <= screenHeight + 100);
            
            if (callCount % 30 == 0 && onScreen) {
                std::cout << "[GameSDK] WorldToScreen success: (" << screenPos.X << ", " << screenPos.Y << ")" << std::endl;
            }
            
            return onScreen;
        }
        catch (const std::exception& e) {
            if (callCount % 100 == 0) {
                std::cout << "[GameSDK] Exception in WorldToScreen: " << e.what() << std::endl;
            }
            return false;
        }
        catch (...) {
            if (callCount % 100 == 0) {
                std::cout << "[GameSDK] Unknown exception in WorldToScreen" << std::endl;
            }
            return false;
        }
    }
    
    bool SimpleWorldToScreen(const FVector& worldPos, FVector2D& screenPos, const FVector& cameraPos) {
        // Fallback simple projection
        FVector diff = worldPos - cameraPos;
        float distance = diff.Distance(FVector());
        
        if (distance < 1.0f) return false;
        
        float screenWidth = (float)GetSystemMetrics(SM_CXSCREEN);
        float screenHeight = (float)GetSystemMetrics(SM_CYSCREEN);
        
        screenPos.X = (screenWidth * 0.5f) + (diff.X / distance) * 400.0f;
        screenPos.Y = (screenHeight * 0.5f) - (diff.Z / distance) * 400.0f;
        
        return (diff.Y > 0 && screenPos.X >= 0 && screenPos.X <= screenWidth && 
               screenPos.Y >= 0 && screenPos.Y <= screenHeight);
    }
}