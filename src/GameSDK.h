#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include <cmath>
#include <iostream>
#include <iomanip>
#include "OffsetLoader.h"

// Global offset macros - these will be dynamically loaded or use fallback values
#define GOBJECTS_OFFSET (g_OffsetLoader ? g_OffsetLoader->GetGObjectsOffset() : 0x89CA6B0)
#define GWORLD_OFFSET (g_OffsetLoader ? g_OffsetLoader->GetGWorldOffset() : 0x8B37258)
#define GNAMES_OFFSET (g_OffsetLoader ? g_OffsetLoader->GetGNamesOffset() : 0x8924300)
#define PROCESSEVENT_OFFSET (g_OffsetLoader ? g_OffsetLoader->GetProcessEventOffset() : 0xE46B80)

// UObject offsets (from CoreUObject_classes.hpp) - these are standard and rarely change
#define UOBJECT_VTABLE_OFFSET 0x0      // VTable pointer
#define UOBJECT_FLAGS_OFFSET 0x8       // EObjectFlags
#define UOBJECT_INDEX_OFFSET 0xC       // Object index
#define UOBJECT_CLASS_OFFSET 0x10      // UClass pointer
#define UOBJECT_NAME_OFFSET 0x18       // FName
#define UOBJECT_OUTER_OFFSET 0x20      // UObject outer

// ---------------- Debug switch ----------------
#ifndef ENABLE_GAMESDK_DEBUG
#define ENABLE_GAMESDK_DEBUG 0
#endif

// ULevel and UWorld offsets - Updated fallback values (0x98 is common in UE 4.27/UE5 for Actors TArray)
#define ULEVEL_ACTORS_OFFSET (g_OffsetLoader ? g_OffsetLoader->GetULevelActorsOffset() : 0x98)
#define UWORLD_PERSISTENTLEVEL_OFFSET (g_OffsetLoader ? g_OffsetLoader->GetUWorldPersistentLevelOffset() : 0x38)
#define UWORLD_GAMEINSTANCE_OFFSET (g_OffsetLoader ? g_OffsetLoader->GetUWorldGameInstanceOffset() : 0x1C0)
#define UGAMEINSTANCE_LOCALPLAYERS_OFFSET (g_OffsetLoader ? g_OffsetLoader->GetUGameInstanceLocalPlayersOffset() : 0x38)
#define ULOCALPLAYER_PLAYERCONTROLLER_OFFSET (g_OffsetLoader ? g_OffsetLoader->GetULocalPlayerPlayerControllerOffset() : 0x30)
#define ACONTROLLER_PAWN_OFFSET (g_OffsetLoader ? g_OffsetLoader->GetAControllerPawnOffset() : 0x2B0)
#define AACTOR_ROOTCOMPONENT_OFFSET (g_OffsetLoader ? g_OffsetLoader->GetAActorRootComponentOffset() : 0x1A0) // was 0x1A8
#define USCENECOMPONENT_RELATIVELOCATION_OFFSET (g_OffsetLoader ? g_OffsetLoader->GetUSceneComponentLocationOffset() : 0x128) // was 0x120

// Basic math structures
struct FVector {
    float X, Y, Z;
    
    FVector() : X(0), Y(0), Z(0) {}
    FVector(float x, float y, float z) : X(x), Y(y), Z(z) {}
    
    FVector operator+(const FVector& other) const {
        return FVector(X + other.X, Y + other.Y, Z + other.Z);
    }
    
    FVector operator-(const FVector& other) const {
        return FVector(X - other.X, Y - other.Y, Z - other.Z);
    }
    
    float Distance(const FVector& other) const {
        FVector diff = *this - other;
        return sqrtf(diff.X * diff.X + diff.Y * diff.Y + diff.Z * diff.Z);
    }
};

struct FVector2D {
    float X, Y;
    
    FVector2D() : X(0), Y(0) {}
    FVector2D(float x, float y) : X(x), Y(y) {}
};

struct FRotator {
    float Pitch, Yaw, Roll;
    
    FRotator() : Pitch(0), Yaw(0), Roll(0) {}
    FRotator(float pitch, float yaw, float roll) : Pitch(pitch), Yaw(yaw), Roll(roll) {}
};

// Forward declarations
class UClass;
class AActor;
class APawn;
class APlayerController;
class ULevel;
class UWorld;

// Forward declare IsValidPointer from GameSDK namespace
namespace GameSDK {
    bool IsValidPointer(void* ptr);
}

// Base UObject class (based on CoreUObject_classes.hpp from RON SDK)
class UObject {
public:
    void* VTable;                    // 0x0000 - Virtual function table
    uint32_t Flags;                  // 0x0008 - EObjectFlags
    int32_t Index;                   // 0x000C - Object index in GObjects array
    UClass* Class;                   // 0x0010 - Pointer to UClass
    uint64_t Name;                   // 0x0018 - FName (8 bytes in UE5)
    UObject* Outer;                  // 0x0020 - Outer object pointer
    
    static uintptr_t GetGObjects() {
        uintptr_t moduleBase = (uintptr_t)GetModuleHandleA(nullptr);
        return moduleBase + GOBJECTS_OFFSET;
    }
    
    static uintptr_t GetGNames() {
        uintptr_t moduleBase = (uintptr_t)GetModuleHandleA(nullptr);
        return moduleBase + GNAMES_OFFSET;
    }
    
    UClass* GetClass() {
        return this->Class;
    }
    
    std::string GetName();
    bool IsA(const std::string& className);
    bool IsValid() {
        return this != nullptr && this->VTable != nullptr;
    }
};

// UClass definition
class UClass : public UObject {
public:
    // Basic UClass implementation
};

// Actor base class (based on Engine_classes.hpp from RON SDK)
class AActor : public UObject {
public:
    // Note: AActor has many members, these are the most important for ESP
    // Full structure available in Engine_classes.hpp from the RON SDK
    
    FVector GetActorLocation() {
        std::cout << "[AActor] GetActorLocation() called on actor: 0x" << std::hex << (uintptr_t)this << std::dec << std::endl;
        
        try {
            // Verify actor pointer is valid
            if (!GameSDK::IsValidPointer(this)) {
                std::cout << "[AActor] ERROR: Invalid actor pointer!" << std::endl;
                return FVector();
            }
            
            // Method 1: Try RootComponent approach
            std::cout << "[AActor] Trying RootComponent method..." << std::endl;
            uintptr_t rootComponent = *(uintptr_t*)((uintptr_t)this + AACTOR_ROOTCOMPONENT_OFFSET);
            if (rootComponent && GameSDK::IsValidPointer((void*)rootComponent)) {
                std::cout << "[AActor] RootComponent found at: 0x" << std::hex << rootComponent << std::dec << std::endl;
                
                // Try different common location offsets
                std::vector<uintptr_t> locationOffsets = {USCENECOMPONENT_RELATIVELOCATION_OFFSET, // 0x128
                                                       USCENECOMPONENT_RELATIVELOCATION_OFFSET + 4, // 0x12C
                                                       USCENECOMPONENT_RELATIVELOCATION_OFFSET + 8};
                
                for (uintptr_t offset : locationOffsets) {
                    FVector* location = (FVector*)(rootComponent + offset);
                    if (GameSDK::IsValidPointer(location)) {
                        FVector loc = *location;
                        // Check if location is reasonable (not zero and not extreme values)
                        if ((loc.X != 0 || loc.Y != 0 || loc.Z != 0) && 
                            abs(loc.X) < 1000000 && abs(loc.Y) < 1000000 && abs(loc.Z) < 1000000) {
                            std::cout << "[AActor] Found valid location at offset 0x" << std::hex << offset << ": (" 
                                     << loc.X << ", " << loc.Y << ", " << loc.Z << ")" << std::dec << std::endl;
                            return loc;
                        }
                    }
                }
            }
            
            // Method 2: Try direct actor location (some actors store location directly)
            std::cout << "[AActor] Trying direct actor location method..." << std::endl;
            std::vector<uintptr_t> actorOffsets = {0x90, 0x94, 0x98, 0x128, 0x12C, 0x130};
            
            for (uintptr_t offset : actorOffsets) {
                FVector* location = (FVector*)((uintptr_t)this + offset);
                if (GameSDK::IsValidPointer(location)) {
                    FVector loc = *location;
                    // Check if location is reasonable
                    if ((loc.X != 0 || loc.Y != 0 || loc.Z != 0) && 
                        abs(loc.X) < 1000000 && abs(loc.Y) < 1000000 && abs(loc.Z) < 1000000) {
                        std::cout << "[AActor] Found valid location at actor offset 0x" << std::hex << offset << ": (" 
                                 << loc.X << ", " << loc.Y << ", " << loc.Z << ")" << std::dec << std::endl;
                        return loc;
                    }
                }
            }
            
            std::cout << "[AActor] Could not find valid location for actor" << std::endl;
            return FVector();
        }
        catch (const std::exception& e) {
            std::cout << "[AActor] Exception in GetActorLocation(): " << e.what() << std::endl;
            return FVector();
        }
        catch (...) {
            std::cout << "[AActor] Unknown exception in GetActorLocation()" << std::endl;
            return FVector();
        }
    }
    
    bool IsValid() {
        return UObject::IsValid();
    }
};

// Pawn class (inherits from Actor)
class APawn : public AActor {
public:
    // Add pawn-specific methods here if needed
};

// Player Controller
class APlayerController : public UObject {
public:
    APawn* GetPawn() {
        return *(APawn**)((uintptr_t)this + 0x2A0); // Typical pawn offset
    }
};

// Level class
class ULevel : public UObject {
public:
    std::vector<AActor*> GetActors() {
        std::vector<AActor*> out;

        if (!GameSDK::IsValidPointer(this)) return out;

        uintptr_t tarrBase = (uintptr_t)this + ULEVEL_ACTORS_OFFSET; // typically 0x98 in UE5
        uintptr_t dataPtr  = *(uintptr_t*)tarrBase;
        int32_t   count    = *(int32_t*)(tarrBase + 0x8);

        if (!dataPtr || count <= 0 || count > 10000) return out;

        for (int32_t i = 0; i < count; ++i) {
            AActor* actor = *(AActor**)(dataPtr + i * sizeof(void*));
            if (actor && GameSDK::IsValidPointer(actor) && actor->IsValid()) {
                out.push_back(actor);
            }
        }

        return out;
    }
};

// World class (based on Engine_classes.hpp from RON SDK)
class UWorld : public UObject {
public:
    // Note: UWorld has many members, structure available in Engine_classes.hpp
    
    static UWorld* GetWorld() {
        std::cout << "[UWorld] GetWorld() called" << std::endl;
        
        try {
            std::cout << "[UWorld] Getting module base..." << std::endl;
            uintptr_t moduleBase = (uintptr_t)GetModuleHandleA(nullptr);
            std::cout << "[UWorld] Module base: 0x" << std::hex << moduleBase << std::dec << std::endl;
            
            std::cout << "[UWorld] Calculating GWorld address..." << std::endl;
            uintptr_t gWorldAddress = moduleBase + GWORLD_OFFSET;
            std::cout << "[UWorld] GWorld address: 0x" << std::hex << gWorldAddress << std::dec << std::endl;
            
            std::cout << "[UWorld] Reading UWorld pointer..." << std::endl;
            UWorld* world = *(UWorld**)(gWorldAddress);
            std::cout << "[UWorld] UWorld pointer: 0x" << std::hex << (uintptr_t)world << std::dec << std::endl;
            
            return world;
        }
        catch (const std::exception& e) {
            std::cout << "[UWorld] Exception in GetWorld(): " << e.what() << std::endl;
            return nullptr;
        }
        catch (...) {
            std::cout << "[UWorld] Unknown exception in GetWorld()" << std::endl;
            return nullptr;
        }
    }
    
    ULevel* GetPersistentLevel() {
        std::cout << "[UWorld] GetPersistentLevel() called on UWorld: 0x" << std::hex << (uintptr_t)this << std::dec << std::endl;
        
        try {
            // Check if this UWorld pointer is valid
            if (!GameSDK::IsValidPointer(this)) {
                std::cout << "[UWorld] ERROR: Invalid UWorld pointer in GetPersistentLevel!" << std::endl;
                return nullptr;
            }
            
            std::cout << "[UWorld] Reading PersistentLevel from offset 0x" << std::hex << UWORLD_PERSISTENTLEVEL_OFFSET << std::dec << std::endl;
            uintptr_t persistentLevelPtr = *(uintptr_t*)((uintptr_t)this + UWORLD_PERSISTENTLEVEL_OFFSET);
            std::cout << "[UWorld] PersistentLevel pointer: 0x" << std::hex << persistentLevelPtr << std::dec << std::endl;
            
            if (!persistentLevelPtr) {
                std::cout << "[UWorld] ERROR: PersistentLevel pointer is NULL!" << std::endl;
                return nullptr;
            }
            
            if (!GameSDK::IsValidPointer((void*)persistentLevelPtr)) {
                std::cout << "[UWorld] ERROR: Invalid PersistentLevel pointer!" << std::endl;
                return nullptr;
            }
            
            std::cout << "[UWorld] PersistentLevel successfully retrieved!" << std::endl;
            return (ULevel*)persistentLevelPtr;
        }
        catch (const std::exception& e) {
            std::cout << "[UWorld] Exception in GetPersistentLevel(): " << e.what() << std::endl;
            return nullptr;
        }
        catch (...) {
            std::cout << "[UWorld] Unknown exception in GetPersistentLevel()" << std::endl;
            return nullptr;
        }
    }
    
    APlayerController* GetFirstPlayerController() {
        try {
            std::cout << "[UWorld] GetFirstPlayerController() called" << std::endl;
            
            // Check if this pointer is valid
            if (!GameSDK::IsValidPointer(this)) {
                std::cout << "[UWorld] ERROR: Invalid UWorld pointer in GetFirstPlayerController!" << std::endl;
                return nullptr;
            }
            
            // OwningGameInstance is at offset 0x1B8 (from SDK: offsetof(UWorld, OwningGameInstance) == 0x0001B8)
            uintptr_t gameInstance = *(uintptr_t*)((uintptr_t)this + 0x1B8);
            std::cout << "[UWorld] GameInstance pointer: 0x" << std::hex << gameInstance << std::dec << std::endl;
            
            if (!gameInstance || !GameSDK::IsValidPointer((void*)gameInstance)) {
                std::cout << "[UWorld] ERROR: Invalid GameInstance pointer!" << std::endl;
                return nullptr;
            }
            
            // LocalPlayers is at offset 0x38 (from SDK: offsetof(UGameInstance, LocalPlayers) == 0x000038)
            uintptr_t localPlayersArray = *(uintptr_t*)(gameInstance + 0x38);
            std::cout << "[UWorld] LocalPlayers array pointer: 0x" << std::hex << localPlayersArray << std::dec << std::endl;
            
            if (!localPlayersArray || !GameSDK::IsValidPointer((void*)localPlayersArray)) {
                std::cout << "[UWorld] ERROR: Invalid LocalPlayers array pointer!" << std::endl;
                return nullptr;
            }
            
            // TArray structure: first 8 bytes is data pointer
            uintptr_t firstLocalPlayer = *(uintptr_t*)localPlayersArray;
            std::cout << "[UWorld] First LocalPlayer pointer: 0x" << std::hex << firstLocalPlayer << std::dec << std::endl;
            
            if (!firstLocalPlayer || !GameSDK::IsValidPointer((void*)firstLocalPlayer)) {
                std::cout << "[UWorld] ERROR: Invalid first LocalPlayer pointer!" << std::endl;
                return nullptr;
            }
            
            // PlayerController offset in ULocalPlayer might be 0x30 or different
            // TODO: Verify this offset from the SDK
            APlayerController* controller = *(APlayerController**)(firstLocalPlayer + 0x30);
            std::cout << "[UWorld] PlayerController pointer: 0x" << std::hex << (uintptr_t)controller << std::dec << std::endl;
            
            return controller;
        }
        catch (const std::exception& e) {
            std::cout << "[UWorld] Exception in GetFirstPlayerController(): " << e.what() << std::endl;
            return nullptr;
        }
        catch (...) {
            std::cout << "[UWorld] Unknown exception in GetFirstPlayerController()" << std::endl;
            return nullptr;
        }
    }
};

// Game SDK helper functions
namespace GameSDK {
    bool IsValidPointer(void* ptr);
    std::vector<AActor*> GetAllActors();
    std::vector<AActor*> GetAllActorsFromGObjects();
    APlayerController* GetLocalPlayerController();
    APawn* GetLocalPlayerPawn();
    FVector GetCameraLocation();
    FRotator GetCameraRotation();
    bool WorldToScreen(const FVector& worldPos, FVector2D& screenPos);
    bool SimpleWorldToScreen(const FVector& worldPos, FVector2D& screenPos, const FVector& cameraPos);
}