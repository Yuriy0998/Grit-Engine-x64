#pragma once
#include "../core/Types.h"
#include "../core/Math.h"

namespace grit::render {

    enum class FogMode : int {
        Linear = 0,
        Exp = 1,
        Exp2 = 2
    };

    struct Environment {
        // Sky
        grit::vec3 skyTopColor = { 0.30f, 0.55f, 0.95f };
        grit::vec3 skyBottomColor = { 0.80f, 0.90f, 1.00f };

        // Sun
        grit::vec3 sunDirection = { 0.0f, -1.0f, 0.0f };    // направление лучей
        grit::vec3 sunColor = { 1.0f,  0.95f, 0.85f };
        f32        sunIntensity = 1.0f;

        // Ambient
        grit::vec3 ambientColor = { 0.5f, 0.55f, 0.6f };
        f32        ambientIntensity = 1.0f;

        // Fog
        bool       fogEnabled = true;
        FogMode    fogMode = FogMode::Linear;
        grit::vec3 fogColor = { 0.78f, 0.82f, 0.88f };
        f32        fogStart = 50.0f;
        f32        fogEnd = 400.0f;
        f32        fogDensity = 0.01f;

        // Time
        f32 timeOfDay = 12.0f; // 0..24
    };

} // namespace grit::render