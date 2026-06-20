#pragma once


#include "render/ImGuiBackend.h"
#include "editor/EditorUI.h"
#include "core/Types.h"
#include "core/Input.h"
#include "rhi/Device.h"
#include "platform/Window.h"
#include "ecs/World.h"
#include "render/Renderer.h"
#include "asset/AssetManager.h"
#include "ecs/SceneSystem.h"
#include "core/Time.h"
#include "core/FileWatcher.h"

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
        bool imguiWantsMouse()    const { return m_imgui.wantCaptureMouse(); }
        bool imguiWantsKeyboard() const { return m_imgui.wantCaptureKeyboard(); }
        
        editor::EditorUI& editorUI() { return m_editor; }
        const editor::EditorUI& editorUI() const { return m_editor; }


        editor::EditorUI& editor() { return m_editor; }
        ecs::World& world() { return m_world; }
        render::Renderer& renderer() { return m_renderer; }
        asset::AssetManager& assets() { return m_assets; }
        Window& window() { return *m_window; }
        Input& input() { return m_input; }
        ecs::SceneSystem& scene() { return m_scene; }
        Time& time() { return m_time; }
        const Time& time() const { return m_time; }
        FileWatcher& fileWatcher() { return m_watcher; }

    private:
        std::unique_ptr<Window>  m_window;
        rhi::Device              m_device;
        ecs::World               m_world;
        render::Renderer         m_renderer;
        asset::AssetManager      m_assets;
        Input                    m_input;
        UpdateFn                 m_onUpdate;
        FileWatcher              m_watcher;

        ecs::SceneSystem m_scene;
        u64                      m_prevTimeNs = 0;
        bool                     m_initialized = false;
        bool                     m_quitRequested = false;
        Time m_time;
        bool m_showStats = false;  // F1 Ч полна€ статистика bgfx
        bool m_wireframe = false;  // F2 Ч wireframe
        
        render::ImGuiBackend m_imgui;
        editor::EditorUI     m_editor;
    };

} // namespace grit