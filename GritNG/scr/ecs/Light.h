#pragma once
#include "../core/Types.h"
#include "../core/Math.h"

namespace grit::ecs {

    // ============================================================
    // Directional Light (солнце)
    //   direction Ч куда светит свет (нормализуетс€ в рендере).
    // ============================================================
    struct LightDirectional {
        vec3 direction{ -0.4f, -0.8f, 0.3f };
        vec3 color{ 1.0f, 1.0f, 1.0f };
        f32  intensity = 1.0f;
        f32  ambient = 0.15f;
    };

    // ============================================================
    // Point Light (точечный)
    //   ѕозици€ берЄтс€ из Transform (Transform.position).
    //   range Ч радиус, после которого свет = 0.
    // ============================================================
    struct LightPoint {
        vec3 color{ 1.0f, 1.0f, 1.0f };
        f32  intensity = 1.0f;
        f32  range = 10.0f;
    };

    // ============================================================
    // Spot Light (прожектор)
    //   ѕозици€ и направление берутс€ из Transform.
    //   direction Ч локальный вектор "вперЄд" (по умолчанию -Y).
    //   innerCone / outerCone Ч углы в радианах:
    //     внутри inner Ч полна€ €ркость,
    //     между inner и outer Ч плавное затухание,
    //     за outer Ч 0.
    // ============================================================
    struct LightSpot {
        vec3 direction{ 0.0f, -1.0f, 0.0f };
        vec3 color{ 1.0f, 1.0f, 1.0f };
        f32  intensity = 1.0f;
        f32  range = 10.0f;
        f32  innerCone = 0.523599f; // 30∞
        f32  outerCone = 0.785398f; // 45∞
    };

} // namespace grit::ecs