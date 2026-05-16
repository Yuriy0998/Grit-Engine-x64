#pragma once
#include "Texture.h"
#include "../asset/AssetHandle.h"
#include <bgfx/bgfx.h>

namespace grit::render {

    // ћинимальный материал: одна albedo-текстура + program
    class Material {
    public:
        asset::AssetHandle<Texture> albedo;
        bgfx::ProgramHandle         program = BGFX_INVALID_HANDLE;
    };

} // namespace grit::render