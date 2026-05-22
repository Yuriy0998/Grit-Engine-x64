#pragma once
#include <memory>

namespace grit::render {

    class StaticMesh;

    namespace Primitives {
        std::shared_ptr<StaticMesh> createCube(float size = 1.0f);
        std::shared_ptr<StaticMesh> createPlane(float size = 10.0f);
        std::shared_ptr<StaticMesh> createSphere(float radius = 0.5f, int segments = 24);
    }

} // namespace grit::render