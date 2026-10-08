#pragma once

// Timer.h — high-resolution frame timer (Lesson 2).
// steady_clock is monotonic: immune to system-clock adjustments, which is
// exactly what you want for frame deltas. The Elapsed()-resets idiom makes
// the Application loop read naturally: float dt = m_Timer.Elapsed();

#include "Pink/Core/Core.h"

#include <chrono>

namespace Pink {

class PINK_API Timer
{
public:
    Timer() { Reset(); }

    void Reset() { m_Start = Clock::now(); }

    // Seconds since construction / last Reset() / last Elapsed().
    // NOTE: Elapsed() rearms the timer — call it once per frame.
    float Elapsed()
    {
        auto now = Clock::now();
        float seconds = std::chrono::duration<float>(now - m_Start).count();
        m_Start = now;
        return seconds;
    }

    float ElapsedMillis()
    {
        auto now = Clock::now();
        float ms = std::chrono::duration<float, std::milli>(now - m_Start).count();
        m_Start = now;
        return ms;
    }

private:
    using Clock = std::chrono::steady_clock;
    Clock::time_point m_Start;
};

}
