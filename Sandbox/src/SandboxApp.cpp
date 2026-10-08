// SandboxApp.cpp — the demo client (Lessons 2–10).
// The engine owns main() (see Core/EntryPoint.h); this file only describes
// the application: its window spec, its subsystems, and its per-frame hooks.
//
// Current state: a textured quad (Lesson 10). The texture is procedural —
// a 256x256 checkerboard generated on the CPU — so the whole pipeline is
// provable with zero asset files. Edit Sandbox/assets/shaders/TexturedQuad.glsl
// while the app runs: the shader hot-reloads (Lesson 8).

#include "Pink/Core/EntryPoint.h" // pulls in Application + Log + main()

#include "Pink/Platform/KeyCodes.h"
#include "Pink/Platform/Input.h"
#include "Pink/Renderer/Renderer.h"
#include "Pink/Renderer/Shader.h"
#include "Pink/Renderer/Buffer.h"
#include "Pink/Renderer/VertexArray.h"
#include "Pink/Renderer/Texture.h"
#include "Pink/Memory/MemoryService.h"

#include <filesystem>

class SandboxApp : public Pink::Application
{
public:
    SandboxApp()
    {
        // Lesson 3+4 wiring: a subsystem owned by the Application,
        // initialized in priority order before the first frame.
        // (Don't touch the service here — OnInit() hasn't run yet, so its
        // allocators don't exist. The ctor only REGISTERS it.)
        AddSubsystem<Pink::MemoryService>();
    }

protected:
    Pink::WindowSpec GetWindowSpec() override
    {
        Pink::WindowSpec spec;
        spec.Title = "PinkMountain Sandbox";
        spec.Width = 1280;
        spec.Height = 720;
        return spec;
    }

    // GPU resources are created HERE — after Renderer::Init() (glad loaded,
    // context current). Creating GL objects in the ctor would run before any
    // context exists: the classic "black screen, no error" bug.
    void OnStart() override
    {
        using namespace Pink;

        // Interleaved: position (vec3) + UV (vec2) per vertex. Stride = 20
        // bytes — computed by BufferLayout, never hand-written.
        float vertices[4 * 5] = {
            -0.5f, -0.5f, 0.0f,   0.0f, 0.0f,
             0.5f, -0.5f, 0.0f,   1.0f, 0.0f,
             0.5f,  0.5f, 0.0f,   1.0f, 1.0f,
            -0.5f,  0.5f, 0.0f,   0.0f, 1.0f
        };

        m_VertexBuffer = VertexBuffer::Create(vertices, sizeof(vertices));
        BufferLayout layout = {
            { ShaderDataType::Float3, "a_Position" },
            { ShaderDataType::Float2, "a_TexCoord" }
        };
        m_VertexBuffer->SetLayout(layout); // BEFORE AddVertexBuffer — the Lesson 9 invariant

        m_VertexArray = VertexArray::Create();
        m_VertexArray->AddVertexBuffer(m_VertexBuffer);

        // Indexed: 4 vertices, 6 indices — no duplicated corners. This is
        // where the EBO earns its keep (Lesson 9).
        uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };
        m_IndexBuffer = IndexBuffer::Create(indices, 6);
        m_VertexArray->SetIndexBuffer(m_IndexBuffer);

        m_Texture = Texture2D::Create(256, 256);
        const uint32_t size = 256, check = 32;
        uint8_t* pixels = new uint8_t[size * size * 4];
        for (uint32_t y = 0; y < size; y++)
            for (uint32_t x = 0; x < size; x++)
            {
                uint8_t c = ((x / check) + (y / check)) % 2 == 0 ? 255 : 0;
                uint8_t* p = pixels + (y * size + x) * 4;
                p[0] = c; p[1] = c; p[2] = c; p[3] = 255; // RGBA bytes
            }
        m_Texture->SetData(pixels, size * size * 4);
        delete[] pixels;

        m_ShaderPath = "assets/shaders/TexturedQuad.glsl";
        m_Shader = Shader::Create(m_ShaderPath);
        m_ShaderWriteTime = std::filesystem::last_write_time(m_ShaderPath);
    }

    // Client per-frame hook — called after subsystem updates, before render.
    void OnUpdate(float deltaTime) override
    {
        using namespace Pink;

        if (Input::IsKeyPressed(KeyCode::Escape))
            RequestShutdown();

        // Lesson 8 hot reload: poll the shader file's write time on the main
        // thread. Every 0.5 s is cheap enough to never show up in a profile,
        // and main-thread polling dodges every file-watcher threading trap.
        m_ReloadTimer += deltaTime;
        if (m_ReloadTimer >= 0.5f)
        {
            m_ReloadTimer = 0.0f;
            auto writeTime = std::filesystem::last_write_time(m_ShaderPath);
            if (writeTime != m_ShaderWriteTime)
            {
                m_ShaderWriteTime = writeTime;
                m_Shader->Reload(); // broken edit? the old program survives (Lesson 8 invariant)
            }
        }

        // Lesson 6 input demo: log polled state once per second.
        m_LogTimer += deltaTime;
        if (m_LogTimer >= 1.0f)
        {
            m_LogTimer = 0.0f;
            if (Input::IsKeyPressed(KeyCode::Left))  PM_INFO("Left arrow held");
            if (Input::IsKeyPressed(KeyCode::Right)) PM_INFO("Right arrow held");
            if (Input::IsKeyPressed(KeyCode::Up))    PM_INFO("Up arrow held");
            if (Input::IsKeyPressed(KeyCode::Down))  PM_INFO("Down arrow held");
            auto [mx, my] = Input::GetMousePosition();
            PM_INFO("Mouse position: ({}, {})", mx, my);
        }
    }

    // The frame's draw calls happen here, in order — between
    // Renderer::BeginFrame and SwapBuffers (Lesson 9).
    void OnRender() override
    {
        m_Texture->Bind(0);
        m_Shader->Bind();
        // The sampler uniform takes the TEXTURE UNIT (0), not the texture
        // handle. Passing the handle is the #1 beginner texture bug.
        m_Shader->SetInt("u_Texture", 0);
        Pink::Renderer::Submit(m_Shader, m_VertexArray);
    }

private:
    Pink::Ref<Pink::Shader> m_Shader;
    Pink::Ref<Pink::VertexBuffer> m_VertexBuffer;
    Pink::Ref<Pink::IndexBuffer> m_IndexBuffer;
    Pink::Ref<Pink::VertexArray> m_VertexArray;
    Pink::Ref<Pink::Texture2D> m_Texture;

    // Hot-reload bookkeeping (Lesson 8)
    std::string m_ShaderPath;
    std::filesystem::file_time_type m_ShaderWriteTime;
    float m_ReloadTimer = 0.0f;

    float m_LogTimer = 0.0f;
};

Pink::Application* Pink::CreateApplication()
{
    PM_INFO("Renderer backend id: {} (1 = OpenGL)", (int)Pink::Renderer::GetAPI());
    return new SandboxApp();
}
