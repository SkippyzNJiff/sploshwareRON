#include "OffsetLoader.h"
#include <filesystem>

// Global instance
OffsetLoader* g_OffsetLoader = nullptr;

OffsetLoader::OffsetLoader() {
    dumpPath = FindDumpFolder();
}

std::string OffsetLoader::FindDumpFolder() {
    // Try to find the dump folder relative to the DLL location
    char modulePath[MAX_PATH];
    GetModuleFileNameA(GetModuleHandle("sploshwareRON.dll"), modulePath, MAX_PATH);
    
    std::filesystem::path dllPath(modulePath);
    std::filesystem::path basePath = dllPath.parent_path().parent_path(); // Go up two levels
    
    // Look for the dump folder
    std::filesystem::path dumpFolder = basePath / "5.3.2-0+Unknown-ReadyOrNot" / "Dumpspace";
    
    if (std::filesystem::exists(dumpFolder)) {
        std::cout << "[OffsetLoader] Found dump folder: " << dumpFolder.string() << std::endl;
        return dumpFolder.string();
    }
    
    // Try current directory
    dumpFolder = std::filesystem::current_path() / "5.3.2-0+Unknown-ReadyOrNot" / "Dumpspace";
    if (std::filesystem::exists(dumpFolder)) {
        std::cout << "[OffsetLoader] Found dump folder: " << dumpFolder.string() << std::endl;
        return dumpFolder.string();
    }
    
    std::cout << "[OffsetLoader] WARNING: Could not find dump folder!" << std::endl;
    return "";
}

std::string OffsetLoader::ReadFileContent(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cout << "[OffsetLoader] Failed to open file: " << filePath << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool OffsetLoader::ExtractOffsetValue(const std::string& content, const std::string& key, uintptr_t& value) {
    // Look for pattern like ["OFFSET_GOBJECTS",150499056]
    std::string pattern = "\\[\"" + key + "\",(\\d+)\\]";
    std::regex regex(pattern);
    std::smatch match;
    
    if (std::regex_search(content, match, regex)) {
        value = std::stoull(match[1].str());
        return true;
    }
    return false;
}

bool OffsetLoader::ExtractClassMemberOffset(const std::string& content, const std::string& className, 
                                           const std::string& memberName, uintptr_t& offset) {
    // First find the class
    std::string classPattern = "\"" + className + "\"\\s*:\\s*\\[";
    std::regex classRegex(classPattern);
    std::smatch classMatch;
    
    if (!std::regex_search(content, classMatch, classRegex)) {
        return false;
    }
    
    // Find the position after the class name
    size_t classPos = classMatch.position() + classMatch.length();
    
    // Look for the member within the class definition
    // Pattern like: "OwningGameInstance":[["UGameInstance","C","*",[]],440,8,1]
    std::string memberPattern = "\"" + memberName + "\"\\s*:\\s*\\[[^\\]]*\\]\\s*,\\s*(\\d+)\\s*,";
    std::regex memberRegex(memberPattern);
    std::smatch memberMatch;
    
    std::string searchContent = content.substr(classPos);
    
    // Find the end of this class (next class definition or end of file)
    size_t endPos = searchContent.find("},\"");
    if (endPos != std::string::npos) {
        searchContent = searchContent.substr(0, endPos);
    }
    
    if (std::regex_search(searchContent, memberMatch, memberRegex)) {
        offset = std::stoull(memberMatch[1].str());
        return true;
    }
    
    return false;
}

bool OffsetLoader::ParseOffsetsInfo() {
    std::string filePath = dumpPath + "\\OffsetsInfo.json";
    std::string content = ReadFileContent(filePath);
    
    if (content.empty()) {
        std::cout << "[OffsetLoader] Failed to read OffsetsInfo.json" << std::endl;
        return false;
    }
    
    std::cout << "[OffsetLoader] Parsing OffsetsInfo.json..." << std::endl;
    
    // Extract global offsets
    uintptr_t value;
    if (ExtractOffsetValue(content, "OFFSET_GOBJECTS", value)) {
        offsets["GOBJECTS"] = value;
        std::cout << "[OffsetLoader] Found GOBJECTS: 0x" << std::hex << value << std::dec << std::endl;
    }
    
    if (ExtractOffsetValue(content, "OFFSET_GNAMES", value)) {
        offsets["GNAMES"] = value;
        std::cout << "[OffsetLoader] Found GNAMES: 0x" << std::hex << value << std::dec << std::endl;
    }
    
    if (ExtractOffsetValue(content, "OFFSET_GWORLD", value)) {
        offsets["GWORLD"] = value;
        std::cout << "[OffsetLoader] Found GWORLD: 0x" << std::hex << value << std::dec << std::endl;
    }
    
    if (ExtractOffsetValue(content, "OFFSET_PROCESSEVENT", value)) {
        offsets["PROCESSEVENT"] = value;
        std::cout << "[OffsetLoader] Found PROCESSEVENT: 0x" << std::hex << value << std::dec << std::endl;
    }
    
    return true;
}

bool OffsetLoader::ParseClassesInfo() {
    std::string filePath = dumpPath + "\\ClassesInfo.json";
    std::string content = ReadFileContent(filePath);
    
    if (content.empty()) {
        std::cout << "[OffsetLoader] Failed to read ClassesInfo.json" << std::endl;
        return false;
    }
    
    std::cout << "[OffsetLoader] Parsing ClassesInfo.json..." << std::endl;
    
    // Extract class member offsets
    uintptr_t offset;
    
    // UWorld members
    if (ExtractClassMemberOffset(content, "UWorld", "OwningGameInstance", offset)) {
        offsets["UWorld::OwningGameInstance"] = offset;
        std::cout << "[OffsetLoader] Found UWorld::OwningGameInstance: 0x" << std::hex << offset << std::dec << std::endl;
    }
    
    if (ExtractClassMemberOffset(content, "UWorld", "PersistentLevel", offset)) {
        offsets["UWorld::PersistentLevel"] = offset;
        std::cout << "[OffsetLoader] Found UWorld::PersistentLevel: 0x" << std::hex << offset << std::dec << std::endl;
    } else {
        std::cout << "[OffsetLoader] WARNING: Could not find UWorld::PersistentLevel offset in ClassesInfo.json" << std::endl;
    }
    
    // UGameInstance members
    if (ExtractClassMemberOffset(content, "UGameInstance", "LocalPlayers", offset)) {
        offsets["UGameInstance::LocalPlayers"] = offset;
        std::cout << "[OffsetLoader] Found UGameInstance::LocalPlayers: 0x" << std::hex << offset << std::dec << std::endl;
    }
    
    // ULocalPlayer members
    if (ExtractClassMemberOffset(content, "ULocalPlayer", "PlayerController", offset)) {
        offsets["ULocalPlayer::PlayerController"] = offset;
        std::cout << "[OffsetLoader] Found ULocalPlayer::PlayerController: 0x" << std::hex << offset << std::dec << std::endl;
    }
    
    // APlayerController/AController members
    if (ExtractClassMemberOffset(content, "APlayerController", "Pawn", offset) ||
        ExtractClassMemberOffset(content, "AController", "Pawn", offset)) {
        offsets["AController::Pawn"] = offset;
        std::cout << "[OffsetLoader] Found AController::Pawn: 0x" << std::hex << offset << std::dec << std::endl;
    }
    
    // ULevel members
    if (ExtractClassMemberOffset(content, "ULevel", "Actors", offset)) {
        offsets["ULevel::Actors"] = offset;
        std::cout << "[OffsetLoader] Found ULevel::Actors: 0x" << std::hex << offset << std::dec << std::endl;
    }
    
    // AActor members
    if (ExtractClassMemberOffset(content, "AActor", "RootComponent", offset)) {
        offsets["AActor::RootComponent"] = offset;
        std::cout << "[OffsetLoader] Found AActor::RootComponent: 0x" << std::hex << offset << std::dec << std::endl;
    }
    
    return true;
}

bool OffsetLoader::LoadOffsets() {
    std::cout << "[OffsetLoader] Loading offsets from dump files..." << std::endl;
    
    if (dumpPath.empty()) {
        std::cout << "[OffsetLoader] ERROR: No dump folder found!" << std::endl;
        return false;
    }
    
    bool success = true;
    
    // Parse all offset files
    if (!ParseOffsetsInfo()) {
        std::cout << "[OffsetLoader] WARNING: Failed to parse OffsetsInfo.json" << std::endl;
        success = false;
    }
    
    if (!ParseClassesInfo()) {
        std::cout << "[OffsetLoader] WARNING: Failed to parse ClassesInfo.json" << std::endl;
        success = false;
    }
    
    // Also check for offset values from Dumper-7 output in the SDK folder
    std::filesystem::path sdkPath = std::filesystem::path(dumpPath).parent_path() / "CppSDK" / "SDK" / "Basic.hpp";
    if (std::filesystem::exists(sdkPath)) {
        std::string sdkContent = ReadFileContent(sdkPath.string());
        if (!sdkContent.empty()) {
            // Extract ULevel::Actors offset from: Off::InSDK::ULevel::Actors: 0x98
            std::regex actorsRegex("Off::InSDK::ULevel::Actors\\s*:\\s*0x([0-9A-Fa-f]+)");
            std::smatch match;
            if (std::regex_search(sdkContent, match, actorsRegex)) {
                offsets["ULevel::Actors"] = std::stoull(match[1].str(), nullptr, 16);
                std::cout << "[OffsetLoader] Found ULevel::Actors from SDK: 0x" << std::hex << offsets["ULevel::Actors"] << std::dec << std::endl;
            }
        }
    }
    
    std::cout << "[OffsetLoader] Loaded " << offsets.size() << " offsets" << std::endl;
    return success && !offsets.empty();
}

uintptr_t OffsetLoader::GetOffset(const std::string& offsetName) const {
    auto it = offsets.find(offsetName);
    if (it != offsets.end()) {
        return it->second;
    }
    return 0;
}

void OffsetLoader::SetOffset(const std::string& name, uintptr_t value) {
    offsets[name] = value;
}

void OffsetLoader::PrintLoadedOffsets() const {
    std::cout << "[OffsetLoader] Loaded offsets:" << std::endl;
    for (const auto& [name, value] : offsets) {
        std::cout << "  " << name << " = 0x" << std::hex << value << std::dec << std::endl;
    }
}

// Getters with fallback values
uintptr_t OffsetLoader::GetGWorldOffset() const {
    uintptr_t offset = GetOffset("GWORLD");
    return offset ? offset : 0x90F3A98; // Fallback from Dumper-7
}

uintptr_t OffsetLoader::GetGObjectsOffset() const {
    uintptr_t offset = GetOffset("GOBJECTS");
    return offset ? offset : 0x8F86EF0; // Fallback from Dumper-7
}

uintptr_t OffsetLoader::GetGNamesOffset() const {
    uintptr_t offset = GetOffset("GNAMES");
    return offset ? offset : 0x8EE0B40; // Fallback from Dumper-7
}

uintptr_t OffsetLoader::GetProcessEventOffset() const {
    uintptr_t offset = GetOffset("PROCESSEVENT");
    return offset ? offset : 0xE76210; // Fallback from Dumper-7
}

uintptr_t OffsetLoader::GetUWorldGameInstanceOffset() const {
    uintptr_t offset = GetOffset("UWorld::OwningGameInstance");
    return offset ? offset : 0x1B8; // Fallback from SDK
}

uintptr_t OffsetLoader::GetUGameInstanceLocalPlayersOffset() const {
    uintptr_t offset = GetOffset("UGameInstance::LocalPlayers");
    return offset ? offset : 0x38; // Fallback from SDK
}

uintptr_t OffsetLoader::GetULocalPlayerPlayerControllerOffset() const {
    uintptr_t offset = GetOffset("ULocalPlayer::PlayerController");
    return offset ? offset : 0x30; // Typical UE5 offset
}

uintptr_t OffsetLoader::GetAControllerPawnOffset() const {
    uintptr_t offset = GetOffset("AController::Pawn");
    return offset ? offset : 0x2A0; // Typical UE5 offset
}

uintptr_t OffsetLoader::GetULevelActorsOffset() const {
    uintptr_t offset = GetOffset("ULevel::Actors");
    return offset ? offset : 0x98; // From Dumper-7
}

uintptr_t OffsetLoader::GetUWorldPersistentLevelOffset() const {
    uintptr_t offset = GetOffset("UWorld::PersistentLevel");
    return offset ? offset : 0x30; // From JSON: 48 decimal = 0x30 hex
}

uintptr_t OffsetLoader::GetAActorRootComponentOffset() const {
    uintptr_t offset = GetOffset("AActor::RootComponent");
    return offset ? offset : 0x198; // Typical UE5 offset
}

uintptr_t OffsetLoader::GetUSceneComponentLocationOffset() const {
    uintptr_t offset = GetOffset("USceneComponent::RelativeLocation");
    return offset ? offset : 0x164; // Typical UE5 offset
} 