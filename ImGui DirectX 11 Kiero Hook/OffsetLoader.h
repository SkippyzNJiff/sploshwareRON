#pragma once
#include <string>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <sstream>
#include <regex>
#include <Windows.h>

class OffsetLoader {
private:
    std::unordered_map<std::string, uintptr_t> offsets;
    std::string dumpPath;
    
    // Simple JSON parsing helpers
    std::string ReadFileContent(const std::string& filePath);
    bool ExtractOffsetValue(const std::string& content, const std::string& key, uintptr_t& value);
    bool ExtractClassMemberOffset(const std::string& content, const std::string& className, const std::string& memberName, uintptr_t& offset);
    
    // Parse OffsetsInfo.json for global offsets
    bool ParseOffsetsInfo();
    
    // Parse ClassesInfo.json for class member offsets
    bool ParseClassesInfo();
    
    // Find the SDK dump folder automatically
    std::string FindDumpFolder();
    
public:
    OffsetLoader();
    
    // Load all offsets from dump files
    bool LoadOffsets();
    
    // Get offset by name, returns 0 if not found
    uintptr_t GetOffset(const std::string& offsetName) const;
    
    // Get all loaded offsets for debugging
    void PrintLoadedOffsets() const;
    
    // Update a specific offset value
    void SetOffset(const std::string& name, uintptr_t value);
    
    // Check if offsets are loaded
    bool IsLoaded() const { return !offsets.empty(); }
    
    // Get offsets with fallback to hardcoded values
    uintptr_t GetGWorldOffset() const;
    uintptr_t GetGObjectsOffset() const;
    uintptr_t GetGNamesOffset() const;
    uintptr_t GetProcessEventOffset() const;
    uintptr_t GetUWorldGameInstanceOffset() const;
    uintptr_t GetUGameInstanceLocalPlayersOffset() const;
    uintptr_t GetULocalPlayerPlayerControllerOffset() const;
    uintptr_t GetAControllerPawnOffset() const;
    uintptr_t GetULevelActorsOffset() const;
    uintptr_t GetUWorldPersistentLevelOffset() const;
    uintptr_t GetAActorRootComponentOffset() const;
    uintptr_t GetUSceneComponentLocationOffset() const;
};

// Global offset loader instance
extern OffsetLoader* g_OffsetLoader; 