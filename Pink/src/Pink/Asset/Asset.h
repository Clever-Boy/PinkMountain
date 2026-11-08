#pragma once

// Asset.h — handles, identity, and the base class every loadable thing
// derives from (Lesson 15).
//
// THE KEY IDEA: game code NEVER refers to an asset by path or by raw
// pointer. It refers to it by AssetHandle — a 64-bit value derived from
// the asset's logical path. Paths are for humans and for the disk; handles
// are for code. Handles are cheap to copy, safe to store in components,
// trivially serializable into a scene file, and — critically — they stay
// valid when the asset is RELOADED: hot reload swaps the object behind
// the handle, and every holder sees the new version without being told.
//
// A handle is a HASH of the normalized logical path (FNV-1a here — fast,
// deterministic, no dependencies). Two consequences to be honest about:
//   * Collisions are theoretically possible (64-bit hash, ~2^32 assets to
//     reach a 50% birthday collision). In practice: never, for a game.
//     The AssetManager still asserts on a registry collision in debug.
//   * The same path always yields the same handle, in every run, on every
//     machine. That is what makes handles serializable — a scene file can
//     store "mesh = 0x9f3c..." and it means the same thing next week.

#include "Pink/Core/Core.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace Pink {

using AssetHandle = uint64_t;
inline constexpr AssetHandle kInvalidAssetHandle = 0;

// FNV-1a 64-bit. Deterministic across runs and platforms (unlike
// std::hash<std::string>, which is implementation-defined).
inline AssetHandle HashAssetPath(std::string_view path)
{
    // Offset basis / prime for FNV-1a 64.
    uint64_t hash = 14695981039346656037ull;
    for (char c : path)
    {
        hash ^= static_cast<uint64_t>(static_cast<unsigned char>(c));
        hash *= 1099511628211ull;
    }
    return hash == kInvalidAssetHandle ? 1 : hash; // 0 is reserved
}

enum class AssetType : uint8_t
{
    None = 0,
    Mesh,
    Texture,
    Shader, // registered for hot reload only — Lesson 8 owns compilation
    Material,
};

// The base class. Every asset knows its handle, its logical path (for
// error messages and for hot reload), and its type. It does NOT know
// about the AssetManager — the manager owns assets, not the reverse.
class PINK_API Asset
{
public:
    Asset(AssetHandle handle, std::string logicalPath, AssetType type)
        : m_Handle(handle), m_LogicalPath(std::move(logicalPath)), m_Type(type) {}
    virtual ~Asset() = default;

    AssetHandle GetHandle() const { return m_Handle; }
    const std::string& GetLogicalPath() const { return m_LogicalPath; }
    AssetType GetType() const { return m_Type; }

    // Hot reload hook: re-read the asset's source file and swap in the
    // new data. The default implementation does nothing (an asset that
    // can't reload simply stays as it is — safe, if stale).
    virtual bool ReloadFromDisk() { return false; }

protected:
    AssetHandle m_Handle;
    std::string m_LogicalPath;
    AssetType m_Type;
};

}
