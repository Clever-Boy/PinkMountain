#include "Pink/Core/Application.h"
#include "Pink/Core/Log.h"
#include "Pink/Core/Assert.h"
#include "Pink/Core/ServiceLocator.h"
#include "Pink/Platform/Window.h"
#include "Pink/Platform/Input.h"
#include "Pink/Renderer/Renderer.h"

#include <algorithm>

namespace Pink {

Application* Application::s_Instance = nullptr;

Application::Application()
{
    PM_CORE_ASSERT(!s_Instance, "Application already exists — only one per process");
    s_Instance = this;
}

Application::~Application()
{
    s_Instance = nullptr;
}

WindowSpec Application::GetWindowSpec()
{
    return WindowSpec{}; // sensible defaults; clients override
}

void Application::Run()
{
    // Platform first: the window owns the OS surface and (from Lesson 7)
    // the GL context the renderer will draw into.
    m_Window = Window::Create(GetWindowSpec());
    Input::SetActiveWindow(m_Window.get());

    // Subsystems, Lesson 3: priority order init...
    std::sort(m_SubSystems.begin(), m_SubSystems.end(),
        [](const Scope<SubSystem>& a, const Scope<SubSystem>& b) {
            return a->GetInitPriority() < b->GetInitPriority();
        });
    for (auto& subsystem : m_SubSystems)
        subsystem->OnInit();

    // Renderer second: gladLoadGL needs the current GL context, which only
    // exists after the window created it. Platform-before-renderer, always.
    Renderer::Init();

    // Platform event → renderer policy. The lambda keeps Window free of
    // renderer knowledge: Window just reports "size changed".
    m_Window->SetResizeCallback([](uint32_t width, uint32_t height) {
        Renderer::OnWindowResize(width, height);
    });

    OnStart(); // client hook (Sandbox): GPU resources are created here, AFTER
               // Renderer::Init() — glad is loaded and a context is current.

    constexpr float FIXED_DT = 1.0f / 60.0f; // Lesson 2: deterministic tick
    float accumulator = 0.0f;

    uint32_t framesThisSecond = 0;
    float fpsTimer = 0.0f;

    m_Timer.Elapsed(); // arm the timer; discard the one-time startup delta
    PM_CORE_INFO("Entering main loop");

    while (m_Running && !m_Window->ShouldClose())
    {
        float deltaTime = m_Timer.Elapsed();

        // Spike clamp: if a hitch costs 2 s, we simulate at most 0.25 s of
        // catch-up. Without this, fixed-update spirals (spiral of death).
        if (deltaTime > 0.25f)
            deltaTime = 0.25f;

        m_Window->OnUpdate(); // pump OS events (close button, resize, keys)

        accumulator += deltaTime;
        while (accumulator >= FIXED_DT)
        {
            for (auto& subsystem : m_SubSystems)
                subsystem->OnFixedUpdate(FIXED_DT);
            accumulator -= FIXED_DT;
        }

        for (auto& subsystem : m_SubSystems)
            subsystem->OnUpdate(deltaTime);

        OnUpdate(deltaTime); // client hook (Sandbox)

        Renderer::BeginFrame({ 0.10f, 0.10f, 0.15f, 1.0f }); // dark slate
        OnRender(); // client hook (Sandbox): the frame's draw calls happen here, in order
        m_Window->SwapBuffers();

        // FPS meter, Lesson 2
        ++framesThisSecond;
        fpsTimer += deltaTime;
        if (fpsTimer >= 1.0f)
        {
            PM_CORE_INFO("FPS: {}", framesThisSecond);
            framesThisSecond = 0;
            fpsTimer = 0.0f;
        }
    }

    Renderer::Shutdown();

    // ...reverse-order shutdown (Lesson 3). Dependents die before dependencies.
    for (auto it = m_SubSystems.rbegin(); it != m_SubSystems.rend(); ++it)
        (*it)->OnShutdown();
    m_SubSystems.clear();

    ServiceLocator::Clear();
}

}
