#pragma once
#include <imgui.h>
#include <memory>
#include <bgfx/bgfx.h>

#include "../render/Material.h"
#include "../render/StaticMesh.h"
#include "../ecs/World.h"

// ВАЖНО: подключите заголовок, где объявлен grit::ecs::MeshRenderer.
// Судя по ошибкам — это НЕ Light.h. Найдите файл с MeshRenderer/Name/Transform.
// Обычно это что-то вроде:
//#include "../ecs/Components.h"   // <-- ЗАМЕНИТЕ на ваш реальный путь
// Если MeshRenderer в отдельном файле:
// #include "../ecs/MeshRenderer.h"

namespace grit::editor::detail {

    // ---- UI layout константы ----
    inline constexpr float kMenuBarH = 24.0f;
    inline constexpr float kTabsH = 44.0f;
    inline constexpr float kToolbarH = 38.0f;
    inline constexpr float kLeftBarW = 44.0f;
    inline constexpr float kTopOffset = kMenuBarH + kTabsH + kToolbarH;

    inline constexpr float kDeg2Rad = 0.01745329252f;
    inline constexpr float kRad2Deg = 57.2957795f;

    // ---- Material helpers ----
    inline std::shared_ptr<grit::render::Material>
        makeDefaultMaterial(bgfx::ProgramHandle prog)
    {
        auto mat = std::make_shared<grit::render::Material>();
        mat->program = prog;
        mat->baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        mat->tilingOffset = { 1.0f, 1.0f, 0.0f, 0.0f };
        mat->pbrParams = { 0.0f, 0.5f, 0.0f, 0.0f };
        mat->emissive = { 0.0f, 0.0f, 0.0f, 0.0f };
        return mat;
    }

    inline void fillDefaultMaterials(grit::ecs::MeshRenderer& mr,
        bgfx::ProgramHandle prog)
    {
        if (!mr.mesh) return;
        const size_t sub = mr.mesh->submeshes().size();
        const size_t n = sub ? sub : 1;
        mr.materials.reserve(n);
        for (size_t i = 0; i < n; ++i)
            mr.materials.push_back(makeDefaultMaterial(prog));
    }

} // namespace grit::editor::detail