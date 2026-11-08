#pragma once

// TextureAsset.h — the Asset wrapper around a GPU texture (Lesson 15).
//
// WHY A WRAPPER: Texture2D (Lesson 10) predates the asset system and
// knows nothing about handles, paths, or reload. Rather than rewriting
// it, we wrap it: TextureAsset IS an Asset (handle/path/type) and OWNS
// a Ref<Texture2D>. The renderer keeps using Texture2D exactly as
// before; the manager deals in TextureAssets.
//
// THE ASYNC SPLIT, MADE EXPLICIT: a texture load has two stages with
// different thread rules —
//   * CPU stage (any thread): read the file bytes, stb-decode them into
//     RGBA pixels. No GL calls anywhere in this stage.
//   * GPU stage (main thread ONLY): Texture2D::Create(w,h) + SetData().
//     GL calls from a worker thread are undefined behaviour on most
//     drivers — this split is the single most important rule in the
//     whole async-loading design, and the AssetManager enforces it by
//     construction (see AssetManager.h).
//
// TextureAsset::Decode() is the CPU stage, callable from a worker.
// TextureAsset::Upload() is the GPU stage, called on the main thread.

#include "Pink/Asset/Asset.h"
#include "Pink/Renderer/Texture.h"

#include <vector>

namespace Pink {

class PINK_API TextureAsset : public Asset
{
public:
    static constexpr AssetType AssetTypeTag = AssetType::Texture;

    TextureAsset(AssetHandle handle, std::string logicalPath)
        : Asset(handle, std::move(logicalPath), AssetType::Texture) {}

    // CPU stage: decode the file into m_Pixels (RGBA8). Safe on any
    // thread. Returns false if the file is missing or undecodable.
    bool DecodeFromFile(const std::string& logicalPath);

    // GPU stage: create the GL texture from the decoded pixels and drop
    // the CPU copy (a shipped texture does not need its pixels twice —
    // the decode buffer is freed here, keeping only the GPU copy).
    // MAIN THREAD ONLY.
    bool UploadToGPU();

    const Ref<Texture2D>& GetTexture() const { return m_Texture; }
    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }

    bool ReloadFromDisk() override
    {
        if (!DecodeFromFile(m_LogicalPath))
            return false;
        return UploadToGPU();
    }

private:
    Ref<Texture2D> m_Texture;
    std::vector<uint8_t> m_Pixels; // CPU stage only; freed by UploadToGPU
    uint32_t m_Width = 0, m_Height = 0;
};

}
