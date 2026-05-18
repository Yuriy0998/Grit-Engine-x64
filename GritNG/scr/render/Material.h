#pragma once
#include "Texture.h"
#include "../asset/AssetHandle.h"
#include "../core/Math.h"
#include <bgfx/bgfx.h>

namespace grit::render {

    // Базовый материал. Текстура + параметры, передаваемые в шейдер.
    class Material {
    public:
        // Ресурсы
        asset::AssetHandle<Texture> albedo;
        bgfx::ProgramHandle         program = BGFX_INVALID_HANDLE;

        // Параметры (передаются в шейдер как uniform'ы)
        vec4 baseColor{ 1.0f, 1.0f, 1.0f, 1.0f };  // rgba: цвет * текстура
        vec4 tilingOffset{ 1.0f, 1.0f, 0.0f, 0.0f };  // xy=scale, zw=offset для uv

        // Зарезервировано под будущий PBR (пока не используется в шейдере)
        vec4 pbrParams{ 0.0f, 1.0f, 0.0f, 0.0f };  // x=metallic, y=roughness, z=,w=
        vec4 emissive{ 0.0f, 0.0f, 0.0f, 0.0f };  // rgb=emissive color, a=intensity
    };

} // namespace grit::render