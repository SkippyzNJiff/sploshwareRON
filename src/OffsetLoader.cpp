#include "OffsetLoader.h"
#include <filesystem>
#include <fstream>
#include <regex>
#include <iostream>
#include <sstream>

// Global instance
OffsetLoader* g_OffsetLoader = nullptr;

OffsetLoader::OffsetLoader() {
    dumpPath = FindDumpFolder();
    if (dumpPath.empty()) {
        std::cout << "[OffsetLoader] ERROR: Dump folder not found; using fallback offsets." << std::endl;
    }
}

std::string OffsetLoader::FindDumpFolder() {
    // Determine DLL path
    char modulePath[MAX_PATH];
    GetModuleFileNameA(GetModuleHandleA("sploshwareRON.dll"), modulePath, MAX_PATH);
    std::filesystem::path dllPath(modulePath);
    std::filesystem::path dllDir  = dllPath.parent_path();          // e.g., x64\Release
    std::filesystem::path x64Dir  = dllDir.parent_path();           // e.g., x64
    std::filesystem::path baseDir = x64Dir.parent_path();           // project root

    // Search roots in order of likelihood
    std::vector<std::filesystem::path> roots = {
        dllDir,       // same folder as DLL (most convenient for users)
        x64Dir,        // parent x64 folder (users often copy jsons here)
        baseDir,       // project root
        std::filesystem::current_path() // whatever the working dir is
    };

    for (const auto& root : roots) {
        if (!std::filesystem::exists(root) || !std::filesystem::is_directory(root)) continue;

        for (auto& entry : std::filesystem::recursive_directory_iterator(root)) {
            if (!entry.is_directory()) continue;
            auto candidate = entry.path();
            auto offsetsFile = candidate / "OffsetsInfo.json";
            auto classesFile = candidate / "ClassesInfo.json";
            auto funcsFile   = candidate / "FunctionsInfo.json";
            if (std::filesystem::exists(offsetsFile) &&
                std::filesystem::exists(classesFile) &&
                std::filesystem::exists(funcsFile)) {
                std::cout << "[OffsetLoader] Found dump folder: " << candidate.string() << std::endl;
                return candidate.string();
            }
        }
    }

    std::cout << "[OffsetLoader] WARNING: Could not find dump folder!" << std::endl;
    return std::string();
}

std::string OffsetLoader::ReadFileContent(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cout << "[OffsetLoader] Failed to open file: " << filePath << std::endl;
        return {};
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool OffsetLoader::ExtractOffsetValue(const std::string& content, const std::string& key, uintptr_t& value) {
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
    std::string classPattern = "\"" + className + "\"\\s*:\\s*\\[";
    std::regex classRegex(classPattern);
    std::smatch classMatch;
    if (!std::regex_search(content, classMatch, classRegex)) return false;

    size_t classPos = classMatch.position() + classMatch.length();
    std::string searchContent = content.substr(classPos);
    size_t endPos = searchContent.find("},\"");
    if (endPos != std::string::npos) searchContent.resize(endPos);

    std::string memberPattern = "\"" + memberName + "\"\\s*:\\s*\\[[^\\]]*\\]\\s*,\\s*(\\d+)";
    std::regex memberRegex(memberPattern);
    std::smatch memberMatch;
    if (std::regex_search(searchContent, memberMatch, memberRegex)) {
        offset = std::stoull(memberMatch[1].str());
        return true;
    }
    return false;
}

bool OffsetLoader::ParseOffsetsInfo() {
    std::string filePath = dumpPath + "/OffsetsInfo.json";
    std::string content = ReadFileContent(filePath);
    if (content.empty()) return false;
    std::cout << "[OffsetLoader] Parsing OffsetsInfo.json..." << std::endl;

    uintptr_t val;
    if (ExtractOffsetValue(content, "OFFSET_GOBJECTS", val)) offsets["GOBJECTS"] = val;
    if (ExtractOffsetValue(content, "OFFSET_GNAMES",    val)) offsets["GNAMES"]    = val;
    if (ExtractOffsetValue(content, "OFFSET_GWORLD",    val)) offsets["GWORLD"]    = val;
    if (ExtractOffsetValue(content, "OFFSET_PROCESSEVENT", val)) offsets["PROCESSEVENT"] = val;
    if (ExtractOffsetValue(content, "OFFSET_APPENDSTRING", val)) offsets["APPENDSTRING"] = val;
    return true;
}

bool OffsetLoader::ParseClassesInfo() {
    std::string filePath = dumpPath + "/ClassesInfo.json";
    std::string content = ReadFileContent(filePath);
    if (content.empty()) return false;
    std::cout << "[OffsetLoader] Parsing ClassesInfo.json..." << std::endl;

    uintptr_t off;
    if (ExtractClassMemberOffset(content, "UWorld",       "OwningGameInstance", off)) offsets["UWorld::OwningGameInstance"] = off;
    if (ExtractClassMemberOffset(content, "UWorld",       "PersistentLevel",     off)) offsets["UWorld::PersistentLevel"]   = off;
    if (ExtractClassMemberOffset(content, "UGameInstance", "LocalPlayers",       off)) offsets["UGameInstance::LocalPlayers"] = off;
    if (ExtractClassMemberOffset(content, "ULocalPlayer",  "PlayerController",    off)) offsets["ULocalPlayer::PlayerController"] = off;
    if (ExtractClassMemberOffset(content, "AController",   "Pawn",                off)) offsets["AController::Pawn"]        = off;
    if (ExtractClassMemberOffset(content, "ULevel",        "Actors",              off)) offsets["ULevel::Actors"]            = off;
    if (ExtractClassMemberOffset(content, "AActor",        "RootComponent",       off)) offsets["AActor::RootComponent"]      = off;
    return true;
}

bool OffsetLoader::LoadOffsets() {
    std::cout << "[OffsetLoader] Loading offsets from dump folder: " << dumpPath << std::endl;
    if (dumpPath.empty()) return false;
    bool ok = ParseOffsetsInfo() & ParseClassesInfo();
    std::cout << "[OffsetLoader] Total offsets loaded: " << offsets.size() << std::endl;
    return ok;
}

// Updated fallback constants for ReadyOrNot 5.3.2 (Dumper-7)
uintptr_t OffsetLoader::GetGWorldOffset() const        { return GetOffsetOrFallback("GWORLD",    0x8B37258); }
uintptr_t OffsetLoader::GetGObjectsOffset() const      { return GetOffsetOrFallback("GOBJECTS",  0x89CA6B0); }
uintptr_t OffsetLoader::GetGNamesOffset() const        { return GetOffsetOrFallback("GNAMES",    0x8924300); }
uintptr_t OffsetLoader::GetProcessEventOffset() const  { return GetOffsetOrFallback("PROCESSEVENT", 0xE46B80); }
uintptr_t OffsetLoader::GetUWorldPersistentLevelOffset() const { return GetOffsetOrFallback("UWorld::PersistentLevel", 0x30); }
uintptr_t OffsetLoader::GetULevelActorsOffset() const  { return GetOffsetOrFallback("ULevel::Actors", 0x98); }

// Helper to get map value or fallback
uintptr_t OffsetLoader::GetOffsetOrFallback(const std::string& key, uintptr_t fallback) const {
    auto it = offsets.find(key);
    return (it != offsets.end() && it->second) ? it->second : fallback;
}

// ----------------- Additional helper / accessor implementations -----------------

uintptr_t OffsetLoader::GetOffset(const std::string& offsetName) const {
    auto it = offsets.find(offsetName);
    return it != offsets.end() ? it->second : 0;
}

void OffsetLoader::PrintLoadedOffsets() const {
    std::cout << "[OffsetLoader] ---- Loaded Offsets ----" << std::endl;
    for (const auto& [name, value] : offsets) {
        std::cout << "[OffsetLoader] " << name << " : 0x" << std::hex << value << std::dec << std::endl;
    }
    std::cout << "[OffsetLoader] ------------------------" << std::endl;
}

void OffsetLoader::SetOffset(const std::string& name, uintptr_t value) {
    offsets[name] = value;
}

uintptr_t OffsetLoader::GetUWorldGameInstanceOffset() const {
    // OwningGameInstance is typically at 0x1B8 in Ready or Not 5.3.2 (UE 4.27/5)
    return GetOffsetOrFallback("UWorld::OwningGameInstance", 0x1B8);
}

uintptr_t OffsetLoader::GetUGameInstanceLocalPlayersOffset() const {
    // LocalPlayers TArray is usually at 0x38
    return GetOffsetOrFallback("UGameInstance::LocalPlayers", 0x38);
}

uintptr_t OffsetLoader::GetULocalPlayerPlayerControllerOffset() const {
    // PlayerController pointer inside ULocalPlayer – generally 0x30
    return GetOffsetOrFallback("ULocalPlayer::PlayerController", 0x30);
}

uintptr_t OffsetLoader::GetAControllerPawnOffset() const {
    // APawn* inside AController – 0x2B0 for this build
    return GetOffsetOrFallback("AController::Pawn", 0x2B0);
}

uintptr_t OffsetLoader::GetAActorRootComponentOffset() const {
    // RootComponent inside AActor – 0x1A0
    return GetOffsetOrFallback("AActor::RootComponent", 0x1A0);
}

uintptr_t OffsetLoader::GetUSceneComponentLocationOffset() const {
    // FVector RelativeLocation inside USceneComponent – 0x128
    return GetOffsetOrFallback("USceneComponent::RelativeLocation", 0x128);
}

// Accessor for AppendString offset (used for converting FName to string)
uintptr_t OffsetLoader::GetAppendStringOffset() const {
    return GetOffsetOrFallback("APPENDSTRING", 0xC8DBB0);
}
