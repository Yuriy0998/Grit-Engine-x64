#pragma once
#include "../core/Types.h"
#include <bgfx/bgfx.h>

namespace grit::render {

    class Texture {
    public:
        Texture() = default;
        ~Texture();

        // Запрещаем копирование
        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        // Загружает любой формат, который понимает bimg
        bool loadFromFile(const char* path);
        void destroy();

        bgfx::TextureHandle handle() const { return m_handle; }
        bool valid() const { return bgfx::isValid(m_handle); }

        u32 width()  const { return m_width; }
        u32 height() const { return m_height; }

    private:
        bgfx::TextureHandle m_handle = BGFX_INVALID_HANDLE;
        u32 m_width = 0;
        u32 m_height = 0;
    };

} // namespace grit::render