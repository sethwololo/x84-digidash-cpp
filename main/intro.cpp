#include "intro.h"

namespace {

constexpr uint32_t FADE_MS = 300;
constexpr uint32_t FADE_STEP_MS = 20;

} // namespace

void Intro::start(uint32_t now_ms)
{
    if (!started_ms_) started_ms_ = now_ms;
}

int Intro::backlight(int target, uint32_t now_ms) const
{
    if (!started_ms_) return 0;
    const uint32_t elapsed = now_ms - *started_ms_;
    if (elapsed >= FADE_MS) return target;
    // Whole steps of FADE_STEP_MS.
    const uint32_t step = elapsed / FADE_STEP_MS;
    constexpr uint32_t STEPS = FADE_MS / FADE_STEP_MS;
    const float k = static_cast<float>(step) / STEPS;
    return static_cast<int>(target * k * k);
}
