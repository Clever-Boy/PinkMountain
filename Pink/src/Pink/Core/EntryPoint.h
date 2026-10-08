#pragma once

// EntryPoint.h - the program's real main() (Lesson 2, Hazel-style).
// The CLIENT (Sandbox) #includes this file and defines CreateApplication().
// Result: the engine owns main(), the game only describes itself.

#include "Pink/Core/Application.h"
#include "Pink/Core/Log.h"

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    Pink::Log::Init();
    PM_CORE_INFO("PinkMountain Engine v0.1.0 - starting up");

    Pink::Application* app = Pink::CreateApplication();
    app->Run();
    delete app;

    PM_CORE_INFO("PinkMountain Engine - shut down cleanly");
    return 0;
}
