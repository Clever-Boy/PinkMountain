#pragma once

// VirtualFileSystem.h — one stable address space for every file the engine
// reads (Lesson 15).
//
// THE PROBLEM IT SOLVES: by Lesson 14 the Sandbox hard-codes paths like
// "assets/shaders/Lit_PBR.glsl". That string means something different
// depending on the working directory — run the .exe from the build folder
// instead of the project root and every load fails. It also bakes the
// on-disk layout into every call site: move the shaders into a .pak file
// later and you touch a hundred lines.
//
// THE VFS ANSWER: code asks for a LOGICAL path ("shaders/Lit_PBR.glsl"
// or "models/cube.obj") and the VFS resolves it against an ordered list
// of MOUNT POINTS. The first mount that contains the file wins, so a
// mod/override directory mounted first shadows the shipped assets —
// the same mechanism real engines use for DLC and user mods.
//
// Mounts are searched in REVERSE registration order (last mounted =
// highest priority). That is the convention: the most recently added
// pack is the one that wins, exactly like PATH or a layered filesystem.
//
// The VFS is deliberately NOT a SubSystem: it must exist before any
// subsystem's OnInit runs (shaders load in OnStart, textures in OnInit).
// It is a plain static service, initialized once from Application::Run
// (or from the Sandbox ctor, as here — see SandboxApp.cpp).

#include "Pink/Core/Core.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace Pink {

class PINK_API VirtualFileSystem
{
public:
    // Mount a real directory at a virtual prefix.
    //   Mount("assets", "Sandbox/assets")   -> "assets/shaders/x.glsl"
    //   Mount("",       "Sandbox/assets")   -> "shaders/x.glsl"
    // Mounting the same physical directory twice is harmless (the first
    // match wins); mounting a missing directory logs a warning but is NOT
    // fatal — a DLC mount may legitimately be absent.
    static void Mount(const std::string& virtualPrefix, const std::string& physicalPath);

    // Resolve a logical path to a physical one. Returns std::nullopt when
    // no mount contains the file — callers must handle that (an assert in
    // debug, a magenta error texture at runtime; never a crash).
    static std::optional<std::filesystem::path> Resolve(const std::string& logicalPath);

    static bool Exists(const std::string& logicalPath) { return Resolve(logicalPath).has_value(); }

    // Whole-file reads. Binary first (the truth), text as a convenience.
    // Returns false on any failure; on success `out` is fully populated.
    static bool ReadFileBinary(const std::string& logicalPath, std::vector<uint8_t>& out);
    static bool ReadFileText(const std::string& logicalPath, std::string& out);

    // Last-write time of the resolved file, for hot reload (Lesson 8's
    // shader watcher grows up into the AssetManager's watcher next file).
    static std::optional<std::filesystem::file_time_type> LastWriteTime(const std::string& logicalPath);

    static void ClearMounts(); // tests only — never in shipped code

private:
    struct MountPoint
    {
        std::string VirtualPrefix; // "" matches everything
        std::filesystem::path PhysicalPath;
    };

    // std::deque would also work; vector + reverse iteration is enough —
    // mount lists are tiny (a handful of entries, not thousands).
    static inline std::vector<MountPoint> s_Mounts;
};

}
