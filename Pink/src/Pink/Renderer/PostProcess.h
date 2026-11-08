#pragma once

// PostProcess.h — the post-processing chain (Lesson 14).
//
// The pipeline, in frame order:
//
//   scene (HDR) ──▶ bright extract (MRT, free) ──▶ blur ×10 (half-res ping-pong)
//        │                                              │
//        └──────────────▶ composite: scene + bloom → exposure → tone map → gamma ──▶ screen
//
// A Lesson-3 SubSystem: registered once, lives for the app's lifetime.
// The demo drives it: BeginScenePass() → render scene → EndScenePass() →
// Execute(). The subsystem owns every target and shader; game code never
// touches a framebuffer directly.
//
// ORDERING INVARIANT (recorded for the 2D/UI lesson): the composite writes
// to the SCREEN. Anything that must not be tone-mapped — ImGui, debug text,
// a crosshair — renders AFTER Execute(), straight to the default framebuffer.
// Tone-mapped UI looks washed out and wrong; this ordering is the reason.

#include "Pink/Core/Core.h"
#include "Pink/Core/SubSystem.h"
#include "Pink/Renderer/Framebuffer.h"
#include "Pink/Renderer/Shader.h"
#include "Pink/Renderer/VertexArray.h"

#include <filesystem>
#include <string>

namespace Pink {

enum class ToneMappingOp
{
    Reinhard = 0,
    Exponential = 1,
    ACES = 2 // Narkowicz approximation — the filmic look
};

class PINK_API PostProcess : public SubSystem
{
public:
    void Init(uint32_t width, uint32_t height);

    // The scene pass renders INTO the HDR target instead of the screen.
    void BeginScenePass(); // bind HDR FBO, viewport, clear
    void EndScenePass();   // unbind — back to the screen

    // Blur the bright target, then composite everything to the screen.
    void Execute();

    void OnResize(uint32_t width, uint32_t height);

    // Lesson-8 pattern, owned here: the demo calls this from its 0.5s
    // reload timer. Broken edit? The old program survives (the invariant).
    void CheckHotReload();

    void SetToneMapping(ToneMappingOp op) { m_ToneMapping = op; }
    ToneMappingOp GetToneMapping() const { return m_ToneMapping; }

    void SetExposure(float ev) { m_Exposure = ev; } // EV stops, 0 = neutral
    float GetExposure() const { return m_Exposure; }

    void SetBloomEnabled(bool enabled) { m_BloomEnabled = enabled; }
    bool IsBloomEnabled() const { return m_BloomEnabled; }
    void SetBloomStrength(float strength) { m_BloomStrength = strength; }

    int GetInitPriority() const override { return 200; }

private:
    void RenderQuad(const Ref<Shader>& shader); // fullscreen triangle via the Lesson-9 seams

private:
    Ref<Framebuffer> m_HDRFramebuffer; // 2x RGBA16F (scene + bright) + depth
    Ref<Framebuffer> m_PingPong[2];    // half-res RGBA16F, no depth
    Ref<Shader> m_BlurShader;
    Ref<Shader> m_CompositeShader;
    Ref<VertexArray> m_QuadVertexArray;

    uint32_t m_Width = 0, m_Height = 0;
    float m_Exposure = 0.0f; // EV stops
    ToneMappingOp m_ToneMapping = ToneMappingOp::ACES;
    bool m_BloomEnabled = true;
    float m_BloomStrength = 0.6f;

    // Hot-reload bookkeeping (Lesson 8), one set per post shader.
    std::string m_BlurPath = "assets/shaders/Blur.glsl";
    std::string m_CompositePath = "assets/shaders/Composite.glsl";
    std::filesystem::file_time_type m_BlurWriteTime;
    std::filesystem::file_time_type m_CompositeWriteTime;
};

}
