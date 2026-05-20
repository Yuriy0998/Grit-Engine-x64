#pragma once
#include "../core/Types.h"

namespace grit {
    class Window;
}

namespace grit::rhi {

    enum class Backend {
        Auto,
        Vulkan,
        D3D11,
        D3D12,
        OpenGL,
    };

    struct DeviceDesc {
        Window* window = nullptr;
        Backend backend = Backend::Auto;
        bool    vsync = true;
        bool    debug = false;
    };

    class Device {
    public:
        bool init(const DeviceDesc& desc);
        void shutdown();

        void beginFrame();
        void endFrame();

        // Пересоздать backbuffer под новый размер окна
        void reset(uint32_t w, uint32_t h);

        // Алиас для совместимости со старым кодом
        void resize(u32 w, u32 h) { reset(w, h); }

        uint32_t width()  const { return m_width; }
        uint32_t height() const { return m_height; }

    private:
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        uint32_t m_resetFlags = 0;
        bool     m_vsync = true;
        bool     m_initialized = false;
    };

} // namespace grit::rhi