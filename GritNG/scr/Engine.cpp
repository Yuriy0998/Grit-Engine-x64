#include "Engine.h"
#include "core/Log.h"

#include <chrono>

namespace grit {

    Engine::Engine() = default;
    Engine::~Engine() { shutdown(); }

    bool Engine::init(const EngineDesc& desc) {
        GRIT_INFO("Engine init...");

        // Window
        m_window = std::make_unique<Window>();
        if (!m_window->create(desc.window)) {
            GRIT_ERROR("Window creation failed");
            return false;
        }
        m_window->setInput(&m_input); // Window будет слать события в Input

        // RHI
        rhi::DeviceDesc dd;
        dd.window = m_window.get();
        dd.backend = desc.backend;
        dd.vsync = desc.vsync;
        dd.debug = desc.debug;
        if (!m_device.init(dd)) {
            GRIT_ERROR("RHI init failed");
            m_window.reset();
            return false;
        }

        // Renderer
        if (!m_renderer.init(&m_device)) {
            GRIT_ERROR("Renderer init failed");
            m_device.shutdown();
            m_window.reset();
            return false;
        }

        m_prevTimeNs = (u64)std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        m_initialized = true;
        GRIT_INFO("Engine init OK");
        return true;
    }

    void Engine::shutdown() {
        if (!m_initialized) return;
        m_assets.clear();
        m_renderer.shutdown();
        m_device.shutdown();
        if (m_window) {
            m_window->destroy();
            m_window.reset();
        }
        m_quitRequested = false;
        m_initialized = false;
        GRIT_INFO("Engine shutdown complete");
    }

    bool Engine::tick() {
        if (!m_initialized) return false;
        if (m_quitRequested) return false;


        // Снимок prev-состояния ввода до обработки сообщений этого кадра
        m_input.beginFrame();

        if (!m_window->pumpMessages()) {
            return false;
        }

        if (m_window->resized()) {
            m_device.resize(m_window->width(), m_window->height());
        }

        // Синхронизируем запрос захвата мыши
        if (m_input._captureRequested() != m_window->isMouseCaptured()) {
            m_window->setMouseCapture(m_input._captureRequested());
        }

        // dt
        u64 now = (u64)std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        f32 dt = (f32)((now - m_prevTimeNs) * 1e-9);
        m_prevTimeNs = now;
        if (dt > 0.25f) dt = 0.25f;

        if (m_onUpdate) m_onUpdate(dt);

        m_device.beginFrame();
        m_renderer.render(m_world, dt);
        m_device.endFrame();

        m_input.endFrame();
        return true;
    }

} // namespace grit