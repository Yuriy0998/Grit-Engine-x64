#include "Device.h"
#include "../platform/Window.h"
#include "../core/Log.h"

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

namespace grit::rhi {

    static bgfx::RendererType::Enum toBgfx(Backend b) {
        switch (b) {
        case Backend::Vulkan: return bgfx::RendererType::Vulkan;
        case Backend::D3D11:  return bgfx::RendererType::Direct3D11;
        case Backend::D3D12:  return bgfx::RendererType::Direct3D12;
        case Backend::OpenGL: return bgfx::RendererType::OpenGL;
        default:              return bgfx::RendererType::Count; // Auto
        }
    }

    bool Device::init(const DeviceDesc& desc) {
        if (!desc.window) {
            GRIT_ERROR("Device::init: no window");
            return false;
        }

        // Сначала отдаём platform data, ещё до bgfx::init
        bgfx::PlatformData pd{};
        pd.nwh = desc.window->nativeHandle();
        bgfx::setPlatformData(pd);

        // Сохраняем reset-флаги: их будем переиспользовать при ресайзе
        m_vsync = desc.vsync;
        m_resetFlags = (desc.vsync ? BGFX_RESET_VSYNC : BGFX_RESET_NONE);

        bgfx::Init init;
        init.type = toBgfx(desc.backend);
        init.resolution.width = desc.window->width();
        init.resolution.height = desc.window->height();
        init.resolution.reset = m_resetFlags;
        init.platformData = pd;
        if (desc.debug) {
            init.debug = true;
        }

        if (!bgfx::init(init)) {
            GRIT_ERROR("bgfx::init failed");
            return false;
        }

        bgfx::setDebug(desc.debug ? BGFX_DEBUG_TEXT : BGFX_DEBUG_NONE);

        bgfx::setViewClear(0,
            BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
            0x303030ff, 1.0f, 0);

        bgfx::setViewRect(0, 0, 0, bgfx::BackbufferRatio::Equal);

        m_width = desc.window->width();
        m_height = desc.window->height();
        m_initialized = true;

        GRIT_INFO("RHI initialized: %s",
            bgfx::getRendererName(bgfx::getRendererType()));
        return true;
    }

    void Device::shutdown() {
        if (m_initialized) {
            bgfx::shutdown();
            m_initialized = false;
            GRIT_INFO("RHI shutdown");
        }
    }

    void Device::beginFrame() {
        bgfx::setViewRect(0, 0, 0, (u16)m_width, (u16)m_height);
        bgfx::touch(0);

        bgfx::dbgTextClear();
        bgfx::dbgTextPrintf(1, 1, 0x0f,
            "GritNG | bgfx %s | %ux%u",
            bgfx::getRendererName(bgfx::getRendererType()),
            m_width, m_height);
    }

    void Device::endFrame() {
        bgfx::frame();
    }

    void Device::reset(uint32_t w, uint32_t h) {
        if (!m_initialized) return;
        if (w == 0 || h == 0) return;
        if (w == m_width && h == m_height) return;

        m_width = w;
        m_height = h;
        bgfx::reset(w, h, m_resetFlags);
        bgfx::setViewRect(0, 0, 0, (u16)w, (u16)h);
    }

} // namespace grit::rhi