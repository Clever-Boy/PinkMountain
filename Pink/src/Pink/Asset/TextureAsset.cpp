// TextureAsset.cpp — CPU decode + GPU upload split (Lesson 15).
//
// stb_image decodes from MEMORY (stbi_load_from_memory), which is what
// makes the split possible: the worker thread reads the file via the VFS
// and decodes it, and the main thread never touches the disk at all.
// This is also why the VFS's ReadFileBinary exists — the texture path
// never goes near stb's fopen.

#include "Pink/Asset/TextureAsset.h"
#include "Pink/Core/Log.h"
#include "Pink/Core/VirtualFileSystem.h"

#include <stb_image.h>

namespace Pink {

bool TextureAsset::DecodeFromFile(const std::string& logicalPath)
{
    std::vector<uint8_t> fileBytes;
    if (!VirtualFileSystem::ReadFileBinary(logicalPath, fileBytes))
        return false;

    // Force 4 channels: the GPU upload path (RGBA8) is then unconditional,
    // and SetData never has to guess the format from the channel count.
    int w = 0, h = 0, channels = 0;
    stbi_set_flip_vertically_on_load(true); // Lesson 10: UV origin is bottom-left
    unsigned char* pixels = stbi_load_from_memory(
        fileBytes.data(), static_cast<int>(fileBytes.size()), &w, &h, &channels, 4);
    if (!pixels)
    {
        PM_CORE_ERROR("TextureAsset: stb failed to decode '{}'", logicalPath);
        return false;
    }
    m_Width = static_cast<uint32_t>(w);
    m_Height = static_cast<uint32_t>(h);
    m_Pixels.assign(pixels, pixels + static_cast<size_t>(w) * static_cast<size_t>(h) * 4);
    stbi_image_free(pixels);
    return true;
}

bool TextureAsset::UploadToGPU()
{
    if (m_Pixels.empty() || m_Width == 0 || m_Height == 0)
        return false;
    m_Texture = Texture2D::Create(m_Width, m_Height);
    m_Texture->SetData(m_Pixels.data(), static_cast<uint32_t>(m_Pixels.size()));
    // Free the CPU copy: the GPU now owns the only copy. (If you later
    // need CPU pixels — e.g. for a lightmap baker — keep them and pay
    // the memory. For a game texture, drop them.)
    m_Pixels.clear();
    m_Pixels.shrink_to_fit();
    return true;
}

}
