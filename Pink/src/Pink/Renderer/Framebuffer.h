#pragma once

// Framebuffer.h — offscreen render targets (Lessons 13–14).
//
// A framebuffer is "a place to render that isn't the screen": a depth texture
// for shadow maps (Lesson 13), a float color texture for HDR (this lesson).
// Lesson 13's spec had a single bool (color on/off). Lesson 14 needs more:
// HOW MANY color targets, and at WHAT PRECISION. Hence ColorFormats —
// a vector, not a flag:
//
//   - empty vector      → depth-only (shadow maps; Lesson-13 GL_NONE path)
//   - { RGBA8 }         → one LDR color target
//   - { RGBA16F, RGBA16F } → two HDR targets (scene + bright, MRT)
//
// Why a VECTOR and not "two bools"? Because the count is the API: the
// backend builds glDrawBuffers from its length, and the lighting shader
// writes layout(location = 0/1). When deferred shading wants 4 G-buffer
// targets in the capstone, this spec doesn't change shape — only length.

#include "Pink/Core/Core.h"

#include <cstdint>
#include <vector>

namespace Pink {

enum class DepthAttachmentType
{
    None = 0,      // no depth (pure post targets: blur ping-pong)
    Texture2D,     // single 2D depth texture (directional shadows, HDR scene)
    CubeMap        // 6-face depth cubemap (point shadows)
};

enum class ColorAttachmentFormat
{
    None = 0,   // placeholder — never stored, keeps the enum honest
    RGBA8,      // 8 bits/channel LDR (the default framebuffer's format)
    RGBA16F     // 16-bit float HDR. NOT RGBA32F: half the bandwidth, and
                // 32F is renderable but NOT texture-filterable on many
                // drivers — 16F filters everywhere. (Lesson math below.)
};

struct FramebufferSpec
{
    uint32_t Width = 0;
    uint32_t Height = 0;
    std::vector<ColorAttachmentFormat> ColorFormats; // empty = depth-only
    DepthAttachmentType DepthType = DepthAttachmentType::None;
};

class PINK_API Framebuffer
{
public:
    virtual ~Framebuffer() = default;

    // Bind for rendering (2D depth, or a color target). Cubemap targets
    // render one face at a time — use BindFace for those.
    virtual void Bind() = 0;
    // Cubemap only: attach one face as the depth target, then render into it.
    // (2D targets: no-op / assert — they have exactly one face.)
    virtual void BindFace(uint32_t face) = 0;
    // Back to the default framebuffer (the screen).
    virtual void Unbind() = 0;

    virtual void Resize(uint32_t width, uint32_t height) = 0;

    // Bind one COLOR attachment for sampling in a later pass (the composite
    // pass samples the HDR scene color; the blur pass samples the bright
    // target). Index = which attachment, slot = texture unit, as usual.
    virtual void BindColorAttachment(uint32_t index, uint32_t slot) const = 0;
    // Bind the DEPTH texture for sampling in a later pass (the lighting
    // pass samples the shadow map). Slot = texture unit, as usual.
    virtual void BindDepthTexture(uint32_t slot = 0) const = 0;

    virtual const FramebufferSpec& GetSpec() const = 0;

    static Ref<Framebuffer> Create(const FramebufferSpec& spec);

protected:
    FramebufferSpec m_Spec;
};

}
