#pragma once

#include "core/Types.h"
#include "core/Input.h"
#include "rhi/Device.h"
#include "platform/Window.h"
#include "ecs/World.h"
#include "render/Renderer.h"
#include "asset/AssetManager.h"

#include <functional>
#include <memory>

namespace grit {

    struct EngineDesc {
        WindowDesc       window;
        rhi::Backend     backend = rhi::Backend::Auto;
        bool             vsync = true;
        bool             debug = true;
    };

    class Engine {
    public:
        using UpdateFn = std::function<void(f32 dt)>;

        Engine();
        ~Engine();

        bool init(const EngineDesc& desc);
        void shutdown();
        bool tick();

        void setOnUpdate(UpdateFn fn) { m_onUpdate = std::move(fn); }

        void requestQuit() { m_quitRequested = true; }
        bool quitRequested() const { return m_quitRequested; }

        ecs::World& world() { return m_world; }
        render::Renderer& renderer() { return m_renderer; }
        asset::AssetManager& assets() { return m_assets; }
        Window& window() { return *m_window; }
        Input& input() { return m_input; }

    private:
        std::unique_ptr<Window>  m_window;
        rhi::Device              m_device;
        ecs::World               m_world;
        render::Renderer         m_renderer;
        asset::AssetManager      m_assets;
        Input                    m_input;
        UpdateFn                 m_onUpdate;

        u64                      m_prevTimeNs = 0;
        bool                     m_initialized = false;
        bool                     m_quitRequested = false;
    };

} // namespace grit