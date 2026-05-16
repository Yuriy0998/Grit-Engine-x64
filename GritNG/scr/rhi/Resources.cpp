#include "Resources.h"
#include "../core/Log.h"
#include <bgfx/bgfx.h>
#include <bx/file.h>
#include <bx/readerwriter.h>
#include <cstdio>
#include <string>

namespace grit::rhi {

    static bgfx::Attrib::Enum toBgfxAttrib(VertexAttrib::Semantic s) {
        switch (s) {
        case VertexAttrib::Semantic::Position:  return bgfx::Attrib::Position;
        case VertexAttrib::Semantic::Normal:    return bgfx::Attrib::Normal;
        case VertexAttrib::Semantic::Color0:    return bgfx::Attrib::Color0;
        case VertexAttrib::Semantic::TexCoord0: return bgfx::Attrib::TexCoord0;
        case VertexAttrib::Semantic::TexCoord1: return bgfx::Attrib::TexCoord1;
        }
        return bgfx::Attrib::Position;
    }

    static bgfx::AttribType::Enum toBgfxType(VertexAttrib::Type t) {
        switch (t) {
        case VertexAttrib::Type::Float: return bgfx::AttribType::Float;
        case VertexAttrib::Type::Uint8: return bgfx::AttribType::Uint8;
        }
        return bgfx::AttribType::Float;
    }

    // ---------------- VertexLayout ----------------

    VertexLayout& VertexLayout::add(VertexAttrib::Semantic s, u8 count,
        VertexAttrib::Type t, bool normalized) {
        m_attribs.push_back({ s, count, t, normalized });
        return *this;
    }

    void VertexLayout::build() {
        if (m_built) return;
        m_layout.begin();
        for (auto& a : m_attribs) {
            m_layout.add(toBgfxAttrib(a.semantic), a.count,
                toBgfxType(a.type), a.normalized);
        }
        m_layout.end();
        m_built = true;
    }

    // ---------------- VertexBuffer ----------------

    bool VertexBuffer::create(const void* data, u32 size, const VertexLayout& layout) {
        m_handle = bgfx::createVertexBuffer(
            bgfx::copy(data, size),
            layout.bgfxLayout());
        return valid();
    }

    void VertexBuffer::destroy() {
        if (valid()) {
            bgfx::destroy(m_handle);
            m_handle = BGFX_INVALID_HANDLE;
        }
    }

    // ---------------- IndexBuffer ----------------

    bool IndexBuffer::create(const void* data, u32 size, bool use32bit) {
        u16 flags = use32bit ? BGFX_BUFFER_INDEX32 : 0;
        m_handle = bgfx::createIndexBuffer(bgfx::copy(data, size), flags);
        m_count = use32bit ? (size / 4) : (size / 2);
        return valid();
    }

    void IndexBuffer::destroy() {
        if (valid()) {
            bgfx::destroy(m_handle);
            m_handle = BGFX_INVALID_HANDLE;
        }
    }

    // ---------------- Shader ----------------

    static const bgfx::Memory* loadFile(const char* path) {
        FILE* f = std::fopen(path, "rb");
        if (!f) {
            GRIT_ERROR("Failed to open file: %s", path);
            return nullptr;
        }
        std::fseek(f, 0, SEEK_END);
        long sz = std::ftell(f);
        std::fseek(f, 0, SEEK_SET);
        const bgfx::Memory* mem = bgfx::alloc((u32)sz + 1);
        std::fread(mem->data, 1, sz, f);
        mem->data[sz] = '\0';
        std::fclose(f);
        return mem;
    }

    bool Shader::loadFromFile(const char* path) {
        const bgfx::Memory* mem = loadFile(path);
        if (!mem) return false;
        m_handle = bgfx::createShader(mem);
        if (!valid()) {
            GRIT_ERROR("createShader failed: %s", path);
            return false;
        }
        bgfx::setName(m_handle, path);
        return true;
    }

    void Shader::destroy() {
        if (valid()) {
            bgfx::destroy(m_handle);
            m_handle = BGFX_INVALID_HANDLE;
        }
    }

    // ---------------- Program ----------------

    bool Program::create(const Shader& vs, const Shader& fs) {
        m_handle = bgfx::createProgram(vs.handle(), fs.handle(), false);
        return valid();
    }

    void Program::destroy() {
        if (valid()) {
            bgfx::destroy(m_handle);
            m_handle = BGFX_INVALID_HANDLE;
        }
    }

    // ---------------- loadProgram ----------------

    // Выбор подкаталога шейдеров по активному бэкенду bgfx
    static const char* shaderDir() {
        switch (bgfx::getRendererType()) {
        case bgfx::RendererType::Direct3D11:
        case bgfx::RendererType::Direct3D12: return "shaders/dx11/";
        case bgfx::RendererType::OpenGL:     return "shaders/glsl/";
        case bgfx::RendererType::OpenGLES:   return "shaders/essl/";
        case bgfx::RendererType::Vulkan:     return "shaders/spirv/";
        case bgfx::RendererType::Metal:      return "shaders/metal/";
        default: break;
        }
        return "shaders/dx11/";
    }

    bgfx::ProgramHandle loadProgram(const char* vsName, const char* fsName) {
        std::string vsPath = std::string(shaderDir()) + vsName + ".bin";
        std::string fsPath = std::string(shaderDir()) + fsName + ".bin";

        Shader vs, fs;
        if (!vs.loadFromFile(vsPath.c_str())) return BGFX_INVALID_HANDLE;
        if (!fs.loadFromFile(fsPath.c_str())) { vs.destroy(); return BGFX_INVALID_HANDLE; }

        bgfx::ProgramHandle prog = bgfx::createProgram(vs.handle(), fs.handle(), true);
        // true = destroy shaders когда program удалится
        return prog;
    }

} // namespace grit::rhi