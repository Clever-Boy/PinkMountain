#pragma once

// Texture.h — the renderer-neutral texture interface (Lesson 10).
//
// A Texture is a 2D array of colors living in VRAM. Vertices carry UV
// coordinates; the fragment shader SAMPLES the texture at the interpolated
// UV. Two creation paths, deliberately: Create(path) for real assets,
// Create(w, h) + SetData for procedural textures (and render targets later).

#include "Pink/Core/Core.h" // Ref<T>

#include <string>

namespace Pink {

class PINK_API Texture
{
public:
    virtual ~Texture() = default;

    virtual uint32_t GetWidth() const = 0;
    virtual uint32_t GetHeight() const = 0;

    virtual void SetData(void* data, uint32_t size) = 0;
    virtual void Bind(uint32_t slot = 0) const = 0;
};

class PINK_API Texture2D : public Texture
{
public:
    static Ref<Texture2D> Create(uint32_t width, uint32_t height);
    static Ref<Texture2D> Create(const std::string& path);
};

}
