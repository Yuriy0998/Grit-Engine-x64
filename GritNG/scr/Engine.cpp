#include "Engine.h"
#include "core/Log.h"
#include <bgfx/bgfx.h>
#include "Editor/EditorStyle.h"


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

        // 1) Сначала ImGui-контекст и бэкенд
        if (!m_imgui.init(/*viewId*/ 200, m_renderer.shaders())) {
            GRIT_ERROR("ImGui init failed");
            return false;
        }

        // 2) ТОЛЬКО ТЕПЕРЬ можно трогать стиль
        grit::editor::applyDarkTheme();

        // 3) Редактор
        m_editor.init(this);

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
        m_imgui.shutdown();
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

        // Реакция на ресайз окна
        const uint32_t winW = m_window->width();
        const uint32_t winH = m_window->height();
        if (winW > 0 && winH > 0 &&
            (winW != m_device.width() || winH != m_device.height()))
        {
            m_device.reset(winW, winH);
        }



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

        // Текстовый оверлей bgfx (оставим — он полезен и не мешает ImGui)
        //bgfx::dbgTextClear();
        //bgfx::dbgTextPrintf(0, 0, 0x0f, "GritNG  |  %.0f FPS  %.2f ms",
        //    m_time.fps(), m_time.msPerFrame());
       // {
       //     const bgfx::Caps* caps = bgfx::getCaps();
       //     const char* rendererName = bgfx::getRendererName(caps->rendererType);
       //     bgfx::dbgTextPrintf(0, 1, 0x08, "Backend: %s  |  Frame: %llu",
       //         rendererName, (unsigned long long)m_time.frameCount());
       // }

        // User update
        if (m_onUpdate) m_onUpdate(dt);

        m_watcher.update(dt);
        m_scene.update(m_world);

        // Рендер сцены
        m_device.beginFrame();
        m_renderer.render(m_world, dt);

        // ImGui поверх
        m_imgui.beginFrame(*m_window, m_input, dt);
        m_editor.draw();
        m_imgui.endFrame();

        m_device.endFrame();

        m_input.beginFrame();
        return !m_quitRequested;
    }

} // namespace grit