#include "GameSDK.h"
#include <iostream>
#include <cmath>

// UObject implementation - Updated for RON 5.3.2-0+Unknown SDK
std::string UObject::GetName() {
    std::cout << "[UObject] GetName() called on object at: 0x" << std::hex << (uintptr_t)this << std::dec << std::endl;
    
    // MASSIVE DEBUGGING - This function can cause crashes when accessing GNames
    // The actual implementation should use the FName structure from the RON SDK
    
    try {
        std::cout << "[UObject] Checking object validity..." << std::endl;
        if (!GameSDK::IsValidPointer(this)) {
            std::cout << "[UObject] ERROR: Invalid UObject pointer!" << std::endl;
            return "InvalidObject";
        }
        
        std::cout << "[UObject] Reading Name value from offset 0x18..." << std::endl;
        // In the new UObject structure, Name is a uint64_t FName at offset 0x18
        uint64_t nameValue = this->Name;
        std::cout << "[UObject] Name value: 0x" << std::hex << nameValue << std::dec << std::endl;
        
        if (nameValue == 0) {
            std::cout << "[UObject] Name value is 0, returning 'None'" << std::endl;
            return "None";
        }
        
        std::cout << "[UObject] Getting GNames array..." << std::endl;
        // Get GNames array
        uintptr_t gNames = GetGNames();
        std::cout << "[UObject] GNames address: 0x" << std::hex << gNames << std::dec << std::endl;
        
        if (!gNames || !GameSDK::IsValidPointer((void*)gNames)) {
            std::cout << "[UObject] ERROR: Invalid GNames pointer!" << std::endl;
            return "InvalidGNames";
        }
        
        // FName structure in UE5: ComparisonIndex (4 bytes) + Number (4 bytes)
        uint32_t comparisonIndex = nameValue & 0xFFFFFFFF;  // Lower 32 bits
        uint32_t number = nameValue >> 32;                  // Upper 32 bits
        std::cout << "[UObject] FName - ComparisonIndex: " << comparisonIndex << ", Number: " << number << std::endl;
        
        // Use the TUObjectArray structure from Basic.hpp for proper GNames access
        // This requires the exact implementation from the RON SDK
        
        std::cout << "[UObject] Object Index: " << this->Index << std::endl;
        // For now, return a safe placeholder
        std::string result = "UObject_" + std::to_string(this->Index);
        std::cout << "[UObject] Returning name: " << result << std::endl;
        return result;
    }
    catch (const std::exception& e) {
        std::cout << "[UObject] Exception in GetName(): " << e.what() << std::endl;
        return "Exception";
    }
    catch (...) {
        std::cout << "[UObject] Unknown exception in GetName()" << std::endl;
        return "Exception";
    }
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
        std::cout << "[GameSDK] IsValidPointer() called with: 0x" << std::hex << (uintptr_t)ptr << std::dec << std::endl;
        
        if (!ptr) {
            std::cout << "[GameSDK] IsValidPointer() - NULL pointer!" << std::endl;
            return false;
        }
        
        std::cout << "[GameSDK] IsValidPointer() - Querying memory info..." << std::endl;
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == 0) {
            std::cout << "[GameSDK] IsValidPointer() - VirtualQuery failed!" << std::endl;
            return false;
        }
        
        std::cout << "[GameSDK] IsValidPointer() - State: 0x" << std::hex << mbi.State << ", Protect: 0x" << mbi.Protect << std::dec << std::endl;
        
        if (mbi.State != MEM_COMMIT) {
            std::cout << "[GameSDK] IsValidPointer() - Memory not committed!" << std::endl;
            return false;
        }
        if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) {
            std::cout << "[GameSDK] IsValidPointer() - Memory protected/no access!" << std::endl;
            return false;
        }
        
        return true;
    }
    
    std::vector<AActor*> GetAllActors() {
        std::vector<AActor*> allActors;
        
        std::cout << "[GameSDK] GetAllActors() called" << std::endl;
        
        try {
            std::cout << "[GameSDK] Attempting to get UWorld..." << std::endl;
            UWorld* world = UWorld::GetWorld();
            if (!world) {
                std::cout << "[GameSDK] ERROR: UWorld is NULL!" << std::endl;
                return allActors;
            }
            if (!IsValidPointer(world)) {
                std::cout << "[GameSDK] ERROR: UWorld pointer is invalid!" << std::endl;
                return allActors;
            }
            std::cout << "[GameSDK] UWorld found at: 0x" << std::hex << (uintptr_t)world << std::dec << std::endl;
            
            std::cout << "[GameSDK] Attempting to get PersistentLevel..." << std::endl;
            ULevel* level = world->GetPersistentLevel();
            if (!level) {
                std::cout << "[GameSDK] ERROR: PersistentLevel is NULL!" << std::endl;
                return allActors;
            }
            if (!IsValidPointer(level)) {
                std::cout << "[GameSDK] ERROR: PersistentLevel pointer is invalid!" << std::endl;
                return allActors;
            }
            std::cout << "[GameSDK] PersistentLevel found at: 0x" << std::hex << (uintptr_t)level << std::dec << std::endl;
            
            std::cout << "[GameSDK] Attempting to get actors from level..." << std::endl;
            
            // Use safe actor retrieval
            std::vector<AActor*> actors = level->GetActors();
            std::cout << "[GameSDK] Retrieved " << actors.size() << " actors from level" << std::endl;
            return actors;
        }
        catch (const std::exception& e) {
            std::cout << "[GameSDK] Exception in GetAllActors(): " << e.what() << std::endl;
            return allActors;
        }
        catch (...) {
            std::cout << "[GameSDK] Unknown exception in GetAllActors()" << std::endl;
            return allActors;
        }
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
        std::cout << "[GameSDK] WorldToScreen called for position: (" << worldPos.X << ", " << worldPos.Y << ", " << worldPos.Z << ")" << std::endl;
        
        try {
            // Get player controller for camera info
            std::cout << "[GameSDK] Getting player controller..." << std::endl;
            APlayerController* controller = GetLocalPlayerController();
            if (!controller) {
                std::cout << "[GameSDK] ERROR: PlayerController is NULL!" << std::endl;
                return false;
            }
            if (!IsValidPointer(controller)) {
                std::cout << "[GameSDK] ERROR: PlayerController pointer is invalid!" << std::endl;
                return false;
            }
            std::cout << "[GameSDK] PlayerController found at: 0x" << std::hex << (uintptr_t)controller << std::dec << std::endl;
            
            // Get camera location and rotation
            FVector cameraLocation = GetCameraLocation();
            
            // For UE5, we need to get the view matrix from the player controller
            // PlayerCameraManager is typically at offset 0x348 in APlayerController
            uintptr_t cameraManager = *(uintptr_t*)((uintptr_t)controller + 0x348);
            if (!cameraManager || !IsValidPointer((void*)cameraManager)) {
                // Fallback to simple projection
                return SimpleWorldToScreen(worldPos, screenPos, cameraLocation);
            }
            
            // Get camera rotation from camera manager (typically at offset 0x1C0)
            FRotator cameraRotation = *(FRotator*)((uintptr_t)cameraManager + 0x1C0);
            
            // Get FOV (typically at offset 0x1CC)
            float fov = *(float*)((uintptr_t)cameraManager + 0x1CC);
            if (fov <= 0 || fov > 180) fov = 90.0f;  // Default FOV
            
            // Convert world position to camera space
            FVector delta = worldPos - cameraLocation;
            
            // Create rotation matrix from camera rotation
            float pitch = cameraRotation.Pitch * (3.14159f / 180.0f);
            float yaw = cameraRotation.Yaw * (3.14159f / 180.0f);
            float roll = cameraRotation.Roll * (3.14159f / 180.0f);
            
            // Rotation matrix calculations
            float cosPitch = cosf(pitch), sinPitch = sinf(pitch);
            float cosYaw = cosf(yaw), sinYaw = sinf(yaw);
            float cosRoll = cosf(roll), sinRoll = sinf(roll);
            
            // Transform to camera space
            FVector cameraSpace;
            cameraSpace.X = delta.X * cosYaw * cosPitch + delta.Y * sinYaw * cosPitch + delta.Z * (-sinPitch);
            cameraSpace.Y = delta.X * (cosYaw * sinPitch * sinRoll - sinYaw * cosRoll) + 
                           delta.Y * (sinYaw * sinPitch * sinRoll + cosYaw * cosRoll) + 
                           delta.Z * (cosPitch * sinRoll);
            cameraSpace.Z = delta.X * (cosYaw * sinPitch * cosRoll + sinYaw * sinRoll) + 
                           delta.Y * (sinYaw * sinPitch * cosRoll - cosYaw * sinRoll) + 
                           delta.Z * (cosPitch * cosRoll);
            
            // Check if behind camera
            if (cameraSpace.X <= 0.1f) return false;
            
            // Get screen dimensions
            float screenWidth = (float)GetSystemMetrics(SM_CXSCREEN);
            float screenHeight = (float)GetSystemMetrics(SM_CYSCREEN);
            
            // Project to screen space
            float fovRadians = fov * (3.14159f / 180.0f);
            float tanHalfFov = tanf(fovRadians * 0.5f);
            
            screenPos.X = (screenWidth * 0.5f) + (cameraSpace.Y / (cameraSpace.X * tanHalfFov)) * (screenWidth * 0.5f);
            screenPos.Y = (screenHeight * 0.5f) - (cameraSpace.Z / (cameraSpace.X * tanHalfFov)) * (screenHeight * 0.5f);
            
            // Check if on screen
            return (screenPos.X >= 0 && screenPos.X <= screenWidth && 
                   screenPos.Y >= 0 && screenPos.Y <= screenHeight);
        }
        catch (...) {
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