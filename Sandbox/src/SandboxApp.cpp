// SandboxApp.cpp — the demo client (Lessons 2–12).
// The engine owns main() (see Core/EntryPoint.h); this file only describes
// the application: its window spec, its subsystems, and its per-frame hooks.
//
// Current state (Lesson 12): three lit cubes — red plastic, brushed steel,
// gold — under one sun + two point lights. Key 1 = Blinn-Phong, key 2 = PBR
// Cook-Torrance. Same scene, same lights: the difference is the model.

#include "Pink/Core/EntryPoint.h" // pulls in Application + Log + main()

#include "Pink/Platform/KeyCodes.h"
#include "Pink/Platform/Input.h"
#include "Pink/Renderer/Renderer.h"
#include "Pink/Renderer/Shader.h"
#include "Pink/Renderer/Buffer.h"
#include "Pink/Renderer/VertexArray.h"
#include "Pink/Renderer/Texture.h"
#include "Pink/Renderer/Camera.h"
#include "Pink/Renderer/Light.h"
#include "Pink/Renderer/Material.h"
#include "Pink/Renderer/LightManager.h"
#include "Pink/Memory/MemoryService.h"

#include <algorithm> // std::clamp
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

namespace {

// Normal matrix, Lesson-12 shortcut: translation-zeroed model matrix.
// Correct for rotation-only and uniform-scale models — for uniform scale s
// the true inverse-transpose equals the 3x3 block divided by s*s (derived in
// the lesson). Non-uniform scale needs the REAL inverse-transpose: that's
// the build task, via Lesson-5's Mat4::Inverse.
Pink::Mat4 MakeNormalMatrix(const Pink::Mat4& model)
{
    Pink::Mat4 n = model;
    n(0, 3) = n(1, 3) = n(2, 3) = 0.0f;
    return n;
}

} // namespace

class SandboxApp : public Pink::Application
{
public:
    SandboxApp()
    {
        AddSubsystem<Pink::MemoryService>();
        m_Lights = &AddSubsystem<Pink::LightManager>();
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

    void OnStart() override
    {
        using namespace Pink;

        // Textured cube, Lesson-12 edition: 24 vertices (4 per face — neither
        // UVs nor NORMALS can be shared across a hard edge, so the 8 geometric
        // corners become 24 shading vertices). Layout is now pos3/normal3/uv2.
        // Winding is the Lesson-11 verified CCW order; each quad gets its face
        // normal on all four vertices (flat shading — smooth normals arrive
        // with real mesh loading).
        struct Face { float nx, ny, nz; float px[4][3]; };
        const Face faces[6] = {
            { 1, 0, 0,  {{ 0.5f,-0.5f, 0.5f},{ 0.5f,-0.5f,-0.5f},{ 0.5f, 0.5f,-0.5f},{ 0.5f, 0.5f, 0.5f}} }, // +X
            {-1, 0, 0,  {{-0.5f,-0.5f,-0.5f},{-0.5f,-0.5f, 0.5f},{-0.5f, 0.5f, 0.5f},{-0.5f, 0.5f,-0.5f}} }, // -X
            { 0, 1, 0,  {{-0.5f, 0.5f, 0.5f},{ 0.5f, 0.5f, 0.5f},{ 0.5f, 0.5f,-0.5f},{-0.5f, 0.5f,-0.5f}} }, // +Y
            { 0,-1, 0,  {{-0.5f,-0.5f,-0.5f},{ 0.5f,-0.5f,-0.5f},{ 0.5f,-0.5f, 0.5f},{-0.5f,-0.5f, 0.5f}} }, // -Y
            { 0, 0, 1,  {{-0.5f,-0.5f, 0.5f},{ 0.5f,-0.5f, 0.5f},{ 0.5f, 0.5f, 0.5f},{-0.5f, 0.5f, 0.5f}} }, // +Z
            { 0, 0,-1,  {{ 0.5f,-0.5f,-0.5f},{-0.5f,-0.5f,-0.5f},{-0.5f, 0.5f,-0.5f},{ 0.5f, 0.5f,-0.5f}} }, // -Z
        };
        const float faceUV[4][2] = { {0,0}, {1,0}, {1,1}, {0,1} };

        std::vector<float> vertices;
        vertices.reserve(24 * 8);
        for (const auto& f : faces)
            for (int v = 0; v < 4; ++v)
            {
                vertices.push_back(f.px[v][0]);
                vertices.push_back(f.px[v][1]);
                vertices.push_back(f.px[v][2]);
                vertices.push_back(f.nx);
                vertices.push_back(f.ny);
                vertices.push_back(f.nz);
                vertices.push_back(faceUV[v][0]);
                vertices.push_back(faceUV[v][1]);
            }

        m_VertexBuffer = VertexBuffer::Create(vertices.data(), (uint32_t)(vertices.size() * sizeof(float)));
        BufferLayout layout = {
            { ShaderDataType::Float3, "a_Position" },
            { ShaderDataType::Float3, "a_Normal" },
            { ShaderDataType::Float2, "a_TexCoord" }
        };
        m_VertexBuffer->SetLayout(layout); // BEFORE AddVertexBuffer — the Lesson 9 invariant

        m_VertexArray = VertexArray::Create();
        m_VertexArray->AddVertexBuffer(m_VertexBuffer);

        uint32_t indices[36];
        for (uint32_t face = 0; face < 6; ++face)
        {
            uint32_t b = face * 4, o = face * 6;
            indices[o + 0] = b + 0; indices[o + 1] = b + 1; indices[o + 2] = b + 2;
            indices[o + 3] = b + 2; indices[o + 4] = b + 3; indices[o + 5] = b + 0;
        }
        m_IndexBuffer = IndexBuffer::Create(indices, 36);
        m_VertexArray->SetIndexBuffer(m_IndexBuffer);

        // The two lighting shaders. Key 1 = Phong, key 2 = PBR.
        m_PhongPath = "assets/shaders/Lit_Phong.glsl";
        m_PBRPath = "assets/shaders/Lit_PBR.glsl";
        m_PhongShader = Shader::Create(m_PhongPath);
        m_PBRShader = Shader::Create(m_PBRPath);
        m_PhongWriteTime = std::filesystem::last_write_time(m_PhongPath);
        m_PBRWriteTime = std::filesystem::last_write_time(m_PBRPath);
        m_ActiveShader = m_PBRShader; // start on PBR — the lesson's destination

        // --- Lesson 12: the scene's lights --------------------------------
        DirectionalLight sun;
        sun.Direction = Normalize(Vec3{ -0.5f, -1.0f, -0.3f });
        sun.Color = Vec3{ 1.0f, 0.96f, 0.90f }; // warm noon sun
        sun.Intensity = 1.2f;
        m_Lights->SetSun(sun);

        PointLight warm; // warm practical, stage right
        warm.Position = Vec3{ 3.0f, 2.0f, 2.0f };
        warm.Color = Vec3{ 1.0f, 0.6f, 0.3f };
        warm.Intensity = 8.0f; // 1/d² eats most of it: at d=4, effective ~= 0.5
        warm.Range = 12.0f;
        m_Lights->AddPointLight(warm);

        PointLight cool; // cool rim, stage left
        cool.Position = Vec3{ -3.0f, 1.5f, -1.0f };
        cool.Color = Vec3{ 0.3f, 0.5f, 1.0f };
        cool.Intensity = 6.0f;
        cool.Range = 12.0f;
        m_Lights->AddPointLight(cool);

        // --- Lesson 12: the three materials --------------------------------
        // Red plastic: dielectric, mid roughness. Gold/steel: metals.
        m_Materials.push_back({ Vec3{ 0.80f, 0.05f, 0.05f }, 0.0f, 0.35f }); // red plastic
        m_Materials.push_back({ Vec3{ 0.70f, 0.72f, 0.75f }, 1.0f, 0.40f }); // brushed steel
        m_Materials.push_back({ Vec3(1.00f, 0.71f, 0.29f) , 1.0f, 0.25f }); // gold
        // (Member init order: Albedo, Metallic, Roughness — AO defaults to 1.)

        AimCamera();
    }

    void OnUpdate(float deltaTime) override
    {
        using namespace Pink;

        if (Input::IsKeyPressed(KeyCode::Escape))
            RequestShutdown();

        // Lighting-model toggle: same scene, same lights — the visible
        // difference is the MODEL. Flip back and forth on the gold cube.
        if (Input::IsKeyPressed(KeyCode::D1)) m_ActiveShader = m_PhongShader;
        if (Input::IsKeyPressed(KeyCode::D2)) m_ActiveShader = m_PBRShader;

        // Orbit controller (Lesson 11, unchanged).
        auto [mx, my] = Input::GetMousePosition();
        if (Input::IsMouseButtonPressed(MouseButton::Left))
        {
            if (m_Dragging)
            {
                m_Yaw   += (mx - m_LastMouseX) * 0.005f;
                m_Pitch -= (my - m_LastMouseY) * 0.005f;
            }
            m_LastMouseX = mx; m_LastMouseY = my;
            m_Dragging = true;
        }
        else
        {
            m_Dragging = false;
        }

        auto [scrollX, scrollY] = Input::TakeScrollOffset();
        (void)scrollX;
        m_Distance = std::clamp(m_Distance - scrollY * 0.5f, 1.5f, 20.0f);

        AimCamera();
        m_Time += deltaTime;

        // Hot reload, both shaders (Lesson 8 pattern, x2).
        m_ReloadTimer += deltaTime;
        if (m_ReloadTimer >= 0.5f)
        {
            m_ReloadTimer = 0.0f;
            ReloadIfChanged(m_PhongShader, m_PhongPath, m_PhongWriteTime);
            ReloadIfChanged(m_PBRShader, m_PBRPath, m_PBRWriteTime);
        }
    }

    void OnRender() override
    {
        using namespace Pink;

        Ref<Shader> shader = m_ActiveShader;
        shader->Bind();
        m_Lights->UploadToShader(*shader);
        shader->SetFloat3("u_CamPos", m_Camera.GetPosition());
        shader->SetMat4("u_ViewProjection", m_Camera.GetViewProjectionMatrix());
        if (shader == m_PhongShader)
            shader->SetFloat("u_Shininess", 32.0f); // Phong-only uniform; the PBR shader has no such name
                                                   // (setting it anyway would only log a warning — the
                                                   // location cache turns unknown names into no-ops)

        // Three cubes, three materials, slow Y spin. Same mesh, same lights —
        // only u_Model / u_NormalMatrix / material change per draw.
        const float xs[3] = { -2.2f, 0.0f, 2.2f };
        for (int i = 0; i < 3; ++i)
        {
            Mat4 model = Translate(Vec3{ xs[i], 0.0f, 0.0f })
                       * Rotate(m_Time * 0.5f, Vec3{ 0.0f, 1.0f, 0.0f });
            shader->SetMat4("u_Model", model);
            shader->SetMat4("u_NormalMatrix", MakeNormalMatrix(model));
            m_Materials[i].Bind(*shader);
            Renderer::Submit(shader, m_VertexArray);
        }
    }

    void OnWindowResize(uint32_t width, uint32_t height) override
    {
        m_Camera.SetAspectRatio(static_cast<float>(width) / static_cast<float>(height));
    }

private:
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

    static void ReloadIfChanged(Pink::Ref<Pink::Shader>& shader,
                                const std::string& path,
                                std::filesystem::file_time_type& writeTime)
    {
        auto t = std::filesystem::last_write_time(path);
        if (t != writeTime)
        {
            writeTime = t;
            shader->Reload(); // broken edit? the old program survives (Lesson 8 invariant)
        }
    }

private:
    Pink::Ref<Pink::Shader> m_PhongShader;
    Pink::Ref<Pink::Shader> m_PBRShader;
    Pink::Ref<Pink::Shader> m_ActiveShader;
    Pink::Ref<Pink::VertexBuffer> m_VertexBuffer;
    Pink::Ref<Pink::IndexBuffer> m_IndexBuffer;
    Pink::Ref<Pink::VertexArray> m_VertexArray;

    Pink::LightManager* m_Lights = nullptr; // owned by the Application (subsystem)
    std::vector<Pink::Material> m_Materials;

    Pink::PerspectiveCamera m_Camera{ 45.0f, 1280.0f / 720.0f, 0.1f, 100.0f };
    Pink::Vec3 m_Target{ 0.0f, 0.0f, 0.0f };
    float m_Yaw = Pink::Radians(45.0f);
    float m_Pitch = Pink::Radians(-20.0f);
    float m_Distance = 6.0f; // widened: three cubes now

    bool m_Dragging = false;
    float m_LastMouseX = 0.0f, m_LastMouseY = 0.0f;
    float m_Time = 0.0f;

    // Hot-reload bookkeeping (Lesson 8), one set per shader.
    std::string m_PhongPath, m_PBRPath;
    std::filesystem::file_time_type m_PhongWriteTime, m_PBRWriteTime;
    float m_ReloadTimer = 0.0f;
};

Pink::Application* Pink::CreateApplication()
{
    PM_INFO("Renderer backend id: {} (1 = OpenGL)", (int)Pink::Renderer::GetAPI());
    return new SandboxApp();
}
