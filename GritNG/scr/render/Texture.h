#pragma once
#include "../core/Types.h"
#include <bgfx/bgfx.h>
#include <string>

namespace grit::render {

    class Texture {
    public:
        Texture() = default;
        ~Texture();

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        bool loadFromFile(const char* path);
        void destroy();

        bgfx::TextureHandle handle() const { return m_handle; }
        bool valid() const { return bgfx::isValid(m_handle); }

        u32 width()  const { return m_width; }
        u32 height() const { return m_height; }

        // Путь к исходному файлу (для сериализации/инспектора)
        const std::string& path() const { return m_path; }
        void setPath(const std::string& p) { m_path = p; }

    private:
        bgfx::TextureHandle m_handle = BGFX_INVALID_HANDLE;
        u32 m_width = 0;
        u32 m_height = 0;
        std::string m_path;
    };

} // namespace grit::render