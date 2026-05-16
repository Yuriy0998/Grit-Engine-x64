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
        bool vsync = true;
        bool debug = false;
    };

    class Device {
    public:
        bool init(const DeviceDesc& desc);
        void shutdown();

        void beginFrame();
        void endFrame();

        void resize(u32 w, u32 h);

        u32 width()  const { return m_width; }
        u32 height() const { return m_height; }

    private:
        u32 m_width = 0;
        u32 m_height = 0;
        bool m_vsync = true;
        bool m_initialized = false;
    };

} // namespace grit::rhi