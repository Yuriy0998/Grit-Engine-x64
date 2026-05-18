#include "Engine.h"
#include "core/Log.h"
#include <bgfx/bgfx.h>

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
        bgfx::setDebug(BGFX_DEBUG_TEXT);
        m_time._reset();

        // Renderer
        if (!m_renderer.init(&m_device, &m_watcher)) {
            GRIT_ERROR("Renderer init failed");
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
        m_watcher.clear();
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
        if (m_quitRequested) return false;
        if (!m_window->pumpMessages()) return false;

        // dt
        using clk = std::chrono::high_resolution_clock;
        static auto prev = clk::now();
        auto now = clk::now();
        f32 rawDt = std::chrono::duration<f32>(now - prev).count();
        prev = now;

        m_time._tick(rawDt);
        f32 dt = m_time.deltaTime();

        // Хоткеи статистики
        if (m_input.wasPressed(Key::F1)) {
            m_showStats = !m_showStats;
            bgfx::setDebug(m_showStats ? BGFX_DEBUG_STATS : BGFX_DEBUG_TEXT);
        }
        if (m_input.wasPressed(Key::F2)) {
            m_wireframe = !m_wireframe;
            bgfx::setDebug(
                (m_showStats ? BGFX_DEBUG_STATS : BGFX_DEBUG_TEXT) |
                (m_wireframe ? BGFX_DEBUG_WIREFRAME : 0)
            );
        }

        // --- Оверлей текста (всегда, поверх рендера) ---
        bgfx::dbgTextClear();
        bgfx::dbgTextPrintf(0, 0, 0x0f, "GritNG  |  %.0f FPS  %.2f ms",
            m_time.fps(), m_time.msPerFrame());
        {
            const bgfx::Caps* caps = bgfx::getCaps();
            const char* rendererName = bgfx::getRendererName(caps->rendererType);
            bgfx::dbgTextPrintf(0, 1, 0x08, "Backend: %s  |  Frame: %llu",
                rendererName, (unsigned long long)m_time.frameCount());
        }
        {
            const bgfx::Stats* st = bgfx::getStats();
            bgfx::dbgTextPrintf(0, 2, 0x08,
                "Draw calls: %d  Tris: %d  GPU: %.2f ms  CPU: %.2f ms",
                st->numDraw,
                (int)(st->numPrims[0] /* triangles */),
                double(st->gpuTimeEnd - st->gpuTimeBegin) * 1000.0 / double(st->gpuTimerFreq),
                double(st->cpuTimeEnd - st->cpuTimeBegin) * 1000.0 / double(st->cpuTimerFreq)
            );
        }
        bgfx::dbgTextPrintf(0, 3, 0x07, "F1 stats   F2 wireframe   Esc quit");

        // User update
        if (m_onUpdate) m_onUpdate(dt);
        
        m_watcher.update(dt);

        // Иерархия → мировые матрицы
        m_scene.update(m_world);

        // Рендер
        m_device.beginFrame();
        m_renderer.render(m_world, dt);
        m_device.endFrame();

        m_input.beginFrame();   // ВАЖНО: после рендера, чтобы wasPressed работал в onUpdate этого кадра
        return !m_quitRequested;
    }

} // namespace grit