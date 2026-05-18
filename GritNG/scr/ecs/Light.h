#pragma once
#include "../core/Types.h"
#include "../core/Math.h"

namespace grit::ecs {

    // Направленный источник света (как солнце).
    // direction — куда светит свет (нормализуется в рендере).
    struct LightDirectional {
        vec3 direction{ -0.4f, -0.8f, 0.3f };
        vec3 color{ 1.0f,  1.0f, 1.0f };
        f32  intensity = 1.0f;
        f32  ambient = 0.15f;  // фоновая составляющая (0..1)
    };

} // namespace grit::ecs