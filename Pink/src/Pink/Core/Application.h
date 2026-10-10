#pragma once

// Application.h — owns the window, the subsystem list, and the main loop
// (Lessons 2, 3, 6, 7).
//
// Loop structure per frame:
//   1. Pump OS events (Window::OnUpdate)
//   2. Fixed-timestep updates: 1/60 s accumulator → SubSystem::OnFixedUpdate
//      (deterministic — physics/AI go here later)
//   3. Variable updates: SubSystem::OnUpdate(dt) with a 0.25 s spike clamp
//      (the spiral-of-death guard: a hitch must not cause a catch-up avalanche)
//   4. Client hook: OnUpdate(dt) — Sandbox overrides this
//   5. Renderer::BeginFrame(clearColor) → OnRender() (client draw calls) → Window::SwapBuffers()
//
// Client lifecycle hooks: OnStart() runs once after Renderer::Init()
// (Lesson 9 — GPU resources are created there, never in the ctor).
//
// IMPORTANT: the Window is created at the top of Run(), NOT in the
// constructor. Calling the virtual GetWindowSpec() from the ctor would
// dispatch to the BASE implementation (C++ rule: virtuals resolve to the
// class under construction) — a classic engine bug.

#include "Pink/Core/Core.h"
#include "Pink/Core/Timer.h"
#include "Pink/Core/SubSystem.h"
#include "Pink/Platform/Window.h"

#include <vector>

namespace Pink {

class PINK_API Application
{
public:
    Application();
    virtual ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void Run();
    void RequestShutdown() { m_Running = false; }

    static Application& Get() { return *s_Instance; }

    // Register a subsystem before Run(). Init order = GetInitPriority()
    // ascending; shutdown is the exact reverse.
    template<typename T, typename... Args>
    T& AddSubsystem(Args&&... args)
    {
        static_assert(std::is_base_of_v<SubSystem, T>, "T must derive from SubSystem");
        auto subsystem = CreateScope<T>(std::forward<Args>(args)...);
        T& ref = *subsystem;
        m_SubSystems.emplace_back(std::move(subsystem));
        return ref;
    }

protected:
    virtual WindowSpec GetWindowSpec();
    virtual void OnStart() {}                       // NEW (Lesson 9): once, after Renderer::Init()
    virtual void OnUpdate(float deltaTime) { (void)deltaTime; }
    virtual void OnRender() {}                      // NEW (Lesson 9): per frame, between BeginFrame and SwapBuffers
    // NEW (Lesson 11): window size changed. Contract: width and height are
    // both > 0 (Application filters the 0x0 minimized case) — safe for
    // aspect-ratio math. Default is a no-op.
    virtual void OnWindowResize(uint32_t width, uint32_t height) { (void)width; (void)height; }

private:
    static Application* s_Instance;

    bool m_Running = true;
    Scope<Window> m_Window;
    Timer m_Timer;
    std::vector<Scope<SubSystem>> m_SubSystems;
};

// Defined by the CLIENT (Sandbox). EntryPoint.h calls it.
Application* CreateApplication();

}
