// SandboxApp.cpp — the demo client (Lessons 2–11).
// The engine owns main() (see Core/EntryPoint.h); this file only describes
// the application: its window spec, its subsystems, and its per-frame hooks.
//
// Current state: a textured, rotating cube (Lesson 11) with a PerspectiveCamera
// and a mouse orbit controller. Left-drag orbits, wheel dollies in/out.

#include "Pink/Core/EntryPoint.h" // pulls in Application + Log + main()

#include "Pink/Platform/KeyCodes.h"
#include "Pink/Platform/Input.h"
#include "Pink/Renderer/Renderer.h"
#include "Pink/Renderer/Shader.h"
#include "Pink/Renderer/Buffer.h"
#include "Pink/Renderer/VertexArray.h"
#include "Pink/Renderer/Texture.h"
#include "Pink/Renderer/Camera.h"
#include "Pink/Memory/MemoryService.h"

#include <algorithm> // std::clamp
#include <cmath>
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

        // Textured cube: 24 vertices (4 per face — UVs can't be shared across
        // a hard edge, so the 8 geometric corners become 24 shading vertices).
        // Winding is CCW seen from outside each face (OpenGL's default front
        // face); verified via Edge1 x Edge2 normals during Lesson 11 authoring.
        float vertices[24 * 5] = {
            // +X face
             0.5f, -0.5f,  0.5f,   0.0f, 0.0f,
             0.5f, -0.5f, -0.5f,   1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,   1.0f, 1.0f,
             0.5f,  0.5f,  0.5f,   0.0f, 1.0f,
            // -X face
            -0.5f, -0.5f, -0.5f,   0.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,   1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,   1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,   0.0f, 1.0f,
            // +Y face
            -0.5f,  0.5f,  0.5f,   0.0f, 0.0f,
             0.5f,  0.5f,  0.5f,   1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,   1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,   0.0f, 1.0f,
            // -Y face
            -0.5f, -0.5f, -0.5f,   0.0f, 0.0f,
             0.5f, -0.5f, -0.5f,   1.0f, 0.0f,
             0.5f, -0.5f,  0.5f,   1.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,   0.0f, 1.0f,
            // +Z face
            -0.5f, -0.5f,  0.5f,   0.0f, 0.0f,
             0.5f, -0.5f,  0.5f,   1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,   1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,   0.0f, 1.0f,
            // -Z face
             0.5f, -0.5f, -0.5f,   0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,   1.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,   1.0f, 1.0f,
             0.5f,  0.5f, -0.5f,   0.0f, 1.0f,
        };

        m_VertexBuffer = VertexBuffer::Create(vertices, sizeof(vertices));
        BufferLayout layout = {
            { ShaderDataType::Float3, "a_Position" },
            { ShaderDataType::Float2, "a_TexCoord" }
        };
        m_VertexBuffer->SetLayout(layout); // BEFORE AddVertexBuffer — the Lesson 9 invariant

        m_VertexArray = VertexArray::Create();
        m_VertexArray->AddVertexBuffer(m_VertexBuffer);

        // 6 faces x 2 triangles. Generated, not hand-typed: the pattern is
        // identical per face, so a loop beats 36 literals (and a typo).
        uint32_t indices[36];
        for (uint32_t face = 0; face < 6; ++face)
        {
            uint32_t b = face * 4, o = face * 6;
            indices[o + 0] = b + 0; indices[o + 1] = b + 1; indices[o + 2] = b + 2;
            indices[o + 3] = b + 2; indices[o + 4] = b + 3; indices[o + 5] = b + 0;
        }
        m_IndexBuffer = IndexBuffer::Create(indices, 36);
        m_VertexArray->SetIndexBuffer(m_IndexBuffer);

        // Same procedural checkerboard as Lesson 10 — zero asset files.
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

        m_ShaderPath = "assets/shaders/Cube.glsl";
        m_Shader = Shader::Create(m_ShaderPath);
        m_ShaderWriteTime = std::filesystem::last_write_time(m_ShaderPath);

        AimCamera(); // place the camera before the first frame
    }

    // Client per-frame hook — called after subsystem updates, before render.
    void OnUpdate(float deltaTime) override
    {
        using namespace Pink;

        if (Input::IsKeyPressed(KeyCode::Escape))
            RequestShutdown();

        // --- Lesson 11: orbit controller --------------------------------
        // Left-drag orbits the target; the wheel dollies in/out. The camera
        // itself only ever receives position + yaw/pitch — orbiting is just
        // spherical coordinates around m_Target, recomputed per frame.
        auto [mx, my] = Input::GetMousePosition();
        if (Input::IsMouseButtonPressed(MouseButton::Left))
        {
            if (m_Dragging)
            {
                float dx = mx - m_LastMouseX, dy = my - m_LastMouseY;
                m_Yaw   += dx * 0.005f; // drag right -> orbit right
                m_Pitch -= dy * 0.005f; // drag up    -> look up
                // Pitch clamp lives in the camera (SetYawPitch) — ±89deg,
                // so the up-vector cross product never degenerates.
            }
            m_LastMouseX = mx; m_LastMouseY = my;
            m_Dragging = true;
        }
        else
        {
            m_Dragging = false; // release: next press re-arms without a jump
        }

        auto [scrollX, scrollY] = Input::TakeScrollOffset();
        (void)scrollX;
        m_Distance = std::clamp(m_Distance - scrollY * 0.5f, 1.5f, 20.0f);

        AimCamera();
        m_Time += deltaTime;

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
        m_Shader->SetMat4("u_ViewProjection", m_Camera.GetViewProjectionMatrix());
        // Slow Y spin so the 3D-ness reads instantly on first launch.
        // Depth testing (enabled in the Lesson-7 renderer init) + the depth
        // clear in BeginFrame are what keep the far faces behind the near ones.
        m_Shader->SetMat4("u_Model", Pink::Rotate(m_Time * 0.5f, Pink::Vec3{ 0.0f, 1.0f, 0.0f }));
        Pink::Renderer::Submit(m_Shader, m_VertexArray);
    }

    // Engine hook (Lesson 11): the window reports size changes; the camera
    // must track the aspect or the image stretches. Application guards the
    // 0x0 minimized case before calling — aspect would divide by zero.
    void OnWindowResize(uint32_t width, uint32_t height) override
    {
        m_Camera.SetAspectRatio(static_cast<float>(width) / static_cast<float>(height));
    }

private:
    // Spherical orbit -> camera position + yaw/pitch. forward(yaw, pitch)
    // points from the camera TOWARD the target, so position = target - f*d
    // and the camera's yaw/pitch are exactly the orbit's yaw/pitch.
    void AimCamera()
    {
        using namespace Pink;
        Vec3 forward{
            std::cos(m_Yaw) * std::cos(m_Pitch),
            std::sin(m_Pitch),
            std::sin(m_Yaw) * std::cos(m_Pitch)
        };
        m_Camera.SetPosition(m_Target - forward * m_Distance);
        m_Camera.SetYawPitch(m_Yaw, m_Pitch);
    }

private:
    Pink::Ref<Pink::Shader> m_Shader;
    Pink::Ref<Pink::VertexBuffer> m_VertexBuffer;
    Pink::Ref<Pink::IndexBuffer> m_IndexBuffer;
    Pink::Ref<Pink::VertexArray> m_VertexArray;
    Pink::Ref<Pink::Texture2D> m_Texture;

    // Lesson 11: the camera + orbit state. Aspect matches the 1280x720 spec
    // above; OnWindowResize keeps it honest afterwards.
    Pink::PerspectiveCamera m_Camera{ 45.0f, 1280.0f / 720.0f, 0.1f, 100.0f };
    Pink::Vec3 m_Target{ 0.0f, 0.0f, 0.0f };
    float m_Yaw = Pink::Radians(45.0f);
    float m_Pitch = Pink::Radians(-20.0f);
    float m_Distance = 4.0f;

    bool m_Dragging = false;
    float m_LastMouseX = 0.0f, m_LastMouseY = 0.0f;
    float m_Time = 0.0f;

    // Hot-reload bookkeeping (Lesson 8)
    std::string m_ShaderPath;
    std::filesystem::file_time_type m_ShaderWriteTime;
    float m_ReloadTimer = 0.0f;
};

Pink::Application* Pink::CreateApplication()
{
    PM_INFO("Renderer backend id: {} (1 = OpenGL)", (int)Pink::Renderer::GetAPI());
    return new SandboxApp();
}
