#include "Texture.h"
#include "../core/Log.h"

#include <bgfx/bgfx.h>
#include <bimg/decode.h>
#include <bx/allocator.h>
#include <bx/file.h>

#include <cstdio>
#include <vector>

namespace grit::render {

    // Простейший allocator на стандартном malloc/free для bimg
    class DefaultAllocator : public bx::AllocatorI {
    public:
        void* realloc(void* _ptr, size_t _size, size_t /*_align*/,
            const char* /*_file*/, uint32_t /*_line*/) override {
            if (_size == 0) {
                std::free(_ptr);
                return nullptr;
            }
            return std::realloc(_ptr, _size);
        }
    };

    static DefaultAllocator g_allocator;

    static bool readFile(const char* path, std::vector<u8>& outBuf) {
        FILE* f = std::fopen(path, "rb");
        if (!f) return false;
        std::fseek(f, 0, SEEK_END);
        long sz = std::ftell(f);
        std::fseek(f, 0, SEEK_SET);
        outBuf.resize(sz);
        std::fread(outBuf.data(), 1, sz, f);
        std::fclose(f);
        return true;
    }

    Texture::~Texture() {
        destroy();
    }

    bool Texture::loadFromFile(const char* path) {
        std::vector<u8> raw;
        if (!readFile(path, raw)) {
            GRIT_ERROR("Texture: cannot open %s", path);
            return false;
        }

        bimg::ImageContainer* img = bimg::imageParse(
            &g_allocator, raw.data(), (u32)raw.size());
        if (!img) {
            GRIT_ERROR("Texture: bimg::imageParse failed: %s", path);
            return false;
        }

        const bgfx::Memory* mem = bgfx::makeRef(
            img->m_data, img->m_size,
            [](void*, void* user) {
                bimg::imageFree(static_cast<bimg::ImageContainer*>(user));
            },
            img);

        u64 flags = BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE;

        if (img->m_cubeMap) {
            m_handle = bgfx::createTextureCube(
                (u16)img->m_width, 1 < img->m_numMips, img->m_numLayers,
                (bgfx::TextureFormat::Enum)img->m_format, flags, mem);
        }
        else if (img->m_depth > 1) {
            m_handle = bgfx::createTexture3D(
                (u16)img->m_width, (u16)img->m_height, (u16)img->m_depth,
                1 < img->m_numMips,
                (bgfx::TextureFormat::Enum)img->m_format, flags, mem);
        }
        else {
            m_handle = bgfx::createTexture2D(
                (u16)img->m_width, (u16)img->m_height,
                1 < img->m_numMips, img->m_numLayers,
                (bgfx::TextureFormat::Enum)img->m_format, flags, mem);
        }

        if (!valid()) {
            GRIT_ERROR("Texture: bgfx::createTexture* failed: %s", path);
            return false;
        }

        m_width = img->m_width;
        m_height = img->m_height;
        bgfx::setName(m_handle, path);

        GRIT_INFO("Texture loaded: %s (%ux%u)", path, m_width, m_height);
        return true;
    }

    void Texture::destroy() {
        if (valid()) {
            bgfx::destroy(m_handle);
            m_handle = BGFX_INVALID_HANDLE;
        }
    }

} // namespace grit::render