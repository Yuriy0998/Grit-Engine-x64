#pragma once
#include "../core/Math.h"
#include "../core/Types.h"

namespace grit::editor {

    struct Ray {
        vec3 origin;
        vec3 dir;   // нормализованный
    };

    Ray  screenToRay(float mouseX, float mouseY,
        float vpW, float vpH,
        const float view[16],
        const float proj[16]);

    bool rayIntersectAABB(const Ray& ray,
        const vec3& aabbMin,
        const vec3& aabbMax,
        float& tOut);

    void transformAABB(const float mtx[16],
        const vec3& inMin, const vec3& inMax,
        vec3& outMin, vec3& outMax);

} // namespace grit::editor