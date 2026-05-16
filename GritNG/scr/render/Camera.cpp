#include "Camera.h"
#include <bx/math.h>

namespace grit::render {

    void Camera::buildView(f32* outView) const {
        const bx::Vec3 eye{ position.x, position.y, position.z };
        const bx::Vec3 at{ target.x,   target.y,   target.z };
        const bx::Vec3 up_{ up.x,       up.y,       up.z };
        bx::mtxLookAt(outView, eye, at, up_);
    }

    void Camera::buildProj(f32* outProj, bool homogeneousNDC) const {
        bx::mtxProj(outProj, fovDegrees, aspect, nearPlane, farPlane, homogeneousNDC);
    }

} // namespace grit::render