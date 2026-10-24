#pragma once

// Framebuffer.h — offscreen render targets (Lesson 13).
//
// A framebuffer is "a place to render that isn't the screen": a depth texture
// for shadow maps (this lesson), a float color texture for HDR (Lesson 14),
// a color+depth pair for post-processing. The abstraction is deliberately
// minimal: one spec, one target, no MRT yet (multiple render targets arrive
// with deferred shading in the capstone).
//
// Why the spec carries HasColorAttachment: a DEPTH-ONLY framebuffer (shadow
// maps) has no color attachment at all — the driver must be told
// glDrawBuffer(GL_NONE), or the framebuffer is incomplete. Lesson 14 flips
// the flag on and gets an HDR color target from the same code.

#include "Pink/Core/Core.h"

#include <cstdint>

namespace Pink {

enum class DepthAttachmentType
{
    None = 0,      // no depth (color-only target)
    Texture2D,     // single 2D depth texture (directional shadows)
    CubeMap        // 6-face depth cubemap (point shadows)
};

struct FramebufferSpec
{
    uint32_t Width = 0;
    uint32_t Height = 0;
    bool HasColorAttachment = false;
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

    // Bind the DEPTH texture for sampling in a later pass (the lighting
    // pass samples the shadow map). Slot = texture unit, as usual.
    virtual void BindDepthTexture(uint32_t slot = 0) const = 0;

    virtual const FramebufferSpec& GetSpec() const = 0;

    static Ref<Framebuffer> Create(const FramebufferSpec& spec);

protected:
    FramebufferSpec m_Spec;
};

}
