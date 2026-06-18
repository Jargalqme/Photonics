#pragma once
#include <DirectXMath.h>
#include <cmath>
#include <algorithm>

class Easing
{
public:
    static float easeInQuad(float t)
    {
        return t * t;
    }

    static float easeOutQuad(float t)
    {
        return 1.0f - (1.0f - t) * (1.0f - t);
    }

    static float easeInOutCubic(float t)
    {
        return t < 0.5f
            ? 4.0f * t * t * t
            : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
    }

    static float easeOutElastic(float t)
    {
        static constexpr float ELASTIC_PERIOD = (2.0f * DirectX::XM_PI) / 3.0f;

        if (t == 0.0f) { return 0.0f; }
        if (t == 1.0f) { return 1.0f; }
        return std::pow(2.0f, -10.0f * t) * sinf((t * 10.0f - 0.75f) * ELASTIC_PERIOD) + 1.0f;
    }

    static float easeOutBack(float t)
    {
        static constexpr float OVERSHOOT = 1.70158f;
        static constexpr float OVERSHOOT_PLUS = OVERSHOOT + 1.0f;

        return 1.0f + OVERSHOOT_PLUS * std::pow(t - 1.0f, 3.0f) + OVERSHOOT * std::pow(t - 1.0f, 2.0f);
    }

    static float easeOutExpo(float t)
    {
        if (t == 1.0f) { return 1.0f; }
        return 1.0f - std::pow(2.0f, -10.0f * t);
    }

    static float smoothstep(float t)
    {
        return t * t * (3.0f - 2.0f * t);
    }

    static float smoothstep(float edge0, float edge1, float t)
    {
        t = std::clamp((t - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }
};
