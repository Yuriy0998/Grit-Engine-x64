#pragma once
#include "../core/Types.h"
#include <bgfx/bgfx.h>
#include <vector>

namespace grit::rhi {

    // Описание одного вершинного атрибута
    struct VertexAttrib {
        enum class Semantic {
            Position,
            Normal,
            Color0,
            TexCoord0,
            TexCoord1,
        };
        enum class Type {
            Float,
            Uint8,
        };

        Semantic semantic;
        u8       count;     // 1..4
        Type     type;
        bool     normalized = false;
    };

    class VertexLayout {
    public:
        VertexLayout& add(VertexAttrib::Semantic s, u8 count,
            VertexAttrib::Type t, bool normalized = false);
        void build();

        const bgfx::VertexLayout& bgfxLayout() const { return m_layout; }

    private:
        bgfx::VertexLayout m_layout;
        std::vector<VertexAttrib> m_attribs;
        bool m_built = false;
    };

    class VertexBuffer {
    public:
        bool create(const void* data, u32 size, const VertexLayout& layout);
        void destroy();

        bgfx::VertexBufferHandle handle() const { return m_handle; }
        bool valid() const { return bgfx::isValid(m_handle); }

    private:
        bgfx::VertexBufferHandle m_handle = BGFX_INVALID_HANDLE;
    };

    class IndexBuffer {
    public:
        bool create(const void* data, u32 size, bool use32bit = false);
        void destroy();

        bgfx::IndexBufferHandle handle() const { return m_handle; }
        bool valid() const { return bgfx::isValid(m_handle); }
        u32  indexCount() const { return m_count; }

    private:
        bgfx::IndexBufferHandle m_handle = BGFX_INVALID_HANDLE;
        u32 m_count = 0;
    };

    class Shader {
    public:
        // Загрузить .bin файл (скомпилированный shaderc-ом bgfx)
        bool loadFromFile(const char* path);
        void destroy();

        bgfx::ShaderHandle handle() const { return m_handle; }
        bool valid() const { return bgfx::isValid(m_handle); }

    private:
        bgfx::ShaderHandle m_handle = BGFX_INVALID_HANDLE;
    };

    class Program {
    public:
        bool create(const Shader& vs, const Shader& fs);
        void destroy();

        bgfx::ProgramHandle handle() const { return m_handle; }
        bool valid() const { return bgfx::isValid(m_handle); }

    private:
        bgfx::ProgramHandle m_handle = BGFX_INVALID_HANDLE;
    };

    // Утилита: загружает vs+fs из папки и собирает program
    // Возвращает bgfx::ProgramHandle (для простоты)
    bgfx::ProgramHandle loadProgram(const char* vsName, const char* fsName);

} // namespace grit::rhi