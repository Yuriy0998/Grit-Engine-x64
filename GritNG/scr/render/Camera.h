#pragma once
#include "../core/Math.h"

namespace grit::render {

    struct Camera {
        vec3 position{ 0.f, 0.f, 5.f };
        vec3 target{ 0.f, 0.f, 0.f };
        vec3 up{ 0.f, 1.f, 0.f };

        f32  fovDegrees = 60.0f;
        f32  aspect = 16.0f / 9.0f;
        f32  nearPlane = 0.1f;
        f32  farPlane = 1000.0f;

        // Заполняет 16 float в column-major для bgfx
        void buildView(f32* outView) const;
        void buildProj(f32* outProj, bool homogeneousNDC) const;
    };

} // namespace grit::render