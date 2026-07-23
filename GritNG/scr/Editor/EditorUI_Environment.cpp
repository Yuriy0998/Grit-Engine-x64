#include "EditorUI.h"
#include "EditorUI_Internal.h"
#include <imgui.h>
#include <bgfx/bgfx.h>

#include "../Engine.h"
#include "../render/Renderer.h"
#include "../render/Renderer_Internal.h"
#include "../render/Environment.h"
#include "../render/TimeOfDay.h"

namespace grit::editor {

    using namespace grit::editor::detail;

    void EditorUI::drawEnvironment()
    {
        if (!ImGui::Begin("Environment", &m_showEnvironment)) {
            ImGui::End();
            return;
        }

        auto& renderer = m_engine->renderer();
        auto& env = renderer.env();

        if (ImGui::CollapsingHeader("Fog", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Enabled", &env.fogEnabled);

            const char* modes[] = { "Linear", "Exp", "Exp2" };
            int mode = (int)env.fogMode;
            if (ImGui::Combo("Mode", &mode, modes, 3))
                env.fogMode = (grit::render::FogMode)mode;

            ImGui::ColorEdit3("Color", &env.fogColor.x);
            ImGui::DragFloat("Start", &env.fogStart, 0.5f, 0.0f, 10000.0f);
            ImGui::DragFloat("End", &env.fogEnd, 0.5f, 0.0f, 10000.0f);
            ImGui::DragFloat("Density", &env.fogDensity, 0.001f, 0.0f, 1.0f, "%.4f");
        }

        if (ImGui::CollapsingHeader("Ambient", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::ColorEdit3("Color", &env.ambientColor.x);
            ImGui::DragFloat("Intensity", &env.ambientIntensity, 0.01f, 0.0f, 5.0f);
        }

        if (ImGui::CollapsingHeader("Sun")) {
            ImGui::DragFloat3("Direction", &env.sunDirection.x, 0.01f, -1.0f, 1.0f);
            ImGui::ColorEdit3("Color", &env.sunColor.x);
            ImGui::DragFloat("Intensity", &env.sunIntensity, 0.01f, 0.0f, 10.0f);
        }

        if (ImGui::CollapsingHeader("Sky (preview)")) {
            ImGui::ColorEdit3("Top", &env.skyTopColor.x);
            ImGui::ColorEdit3("Bottom", &env.skyBottomColor.x);
        }

        // ===== Shadows =====
        if (ImGui::CollapsingHeader("Shadows", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Enabled", &renderer.shadowsEnabled());
            ImGui::Checkbox("Soft (PCF 3x3)", &renderer.softShadows());
            ImGui::DragFloat("Distance", &renderer.shadowDistance(), 0.5f, 5.0f, 500.0f, "%.1f m");
            ImGui::DragFloat("Bias", &renderer.shadowBias(), 0.0001f, 0.0f, 0.05f, "%.4f");

            static const int   sizes[] = { 512, 1024, 2048, 4096 };
            static const char* sizeNames[] = { "512", "1024", "2048", "4096" };
            int current = 2;
            for (int i = 0; i < 4; ++i) {
                if ((u32)sizes[i] == renderer.shadowMapSize()) { current = i; break; }
            }
            if (ImGui::Combo("Map Size", &current, sizeNames, 4))
                renderer.setShadowMapSize((u32)sizes[current]);

            ImGui::Separator();
            ImGui::TextDisabled("Shadow Map Preview:");
            if (bgfx::isValid(renderer.shadowMapTexture())) {
                const float avail = ImGui::GetContentRegionAvail().x;
                const float side = avail < 256.0f ? avail : 256.0f;
                ImTextureID id = (ImTextureID)(uintptr_t)renderer.shadowMapTexture().idx;
                ImGui::Image(id, ImVec2(side, side));
            }
            else {
                ImGui::TextDisabled("<no shadow map>");
            }
        }

        if (ImGui::CollapsingHeader("Tone Mapping", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat("Exposure", &renderer.exposure(), 0.01f, 0.0f, 10.0f);
            const char* ops[] = { "Reinhard", "ACES", "None (clamp)" };
            ImGui::Combo("Operator", &renderer.tonemapOperator(), ops, 3);
            ImGui::DragFloat("Gamma", &renderer.gamma(), 0.01f, 1.0f, 3.0f);
        }

        if (ImGui::CollapsingHeader("Bloom", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Enabled", &renderer.bloomEnabled());
            ImGui::DragFloat("Threshold", &renderer.bloomThreshold(), 0.01f, 0.0f, 10.0f);
            ImGui::DragFloat("Knee (soft)", &renderer.bloomKnee(), 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Intensity", &renderer.bloomIntensity(), 0.01f, 0.0f, 5.0f);
            ImGui::DragFloat("Filter Radius", &renderer.bloomFilterRadius(), 0.01f, 0.1f, 5.0f);
        }

        ImGui::End();
    }

    void EditorUI::drawTimeOfDay()
    {
        if (!ImGui::Begin("Time of Day", &m_showTimeOfDay)) {
            ImGui::End();
            return;
        }

        auto& tod = m_engine->renderer().timeOfDay();

        f32 hour = tod.hour();
        if (ImGui::SliderFloat("Hour", &hour, 0.0f, 24.0f, "%.2f h"))
            tod.setHour(hour);

        f32 speed = tod.speed();
        if (ImGui::DragFloat("Speed (hours/sec)", &speed, 0.01f, 0.0f, 5.0f))
            tod.setSpeed(speed);

        ImGui::Separator();

        if (ImGui::Button("Pause"))          tod.setSpeed(0.0f);
        ImGui::SameLine();
        if (ImGui::Button("Real (24min)"))   tod.setSpeed(1.0f / 60.0f);
        ImGui::SameLine();
        if (ImGui::Button("Fast (24sec)"))   tod.setSpeed(1.0f);
        ImGui::SameLine();
        if (ImGui::Button("Crazy (2.4sec)")) tod.setSpeed(10.0f);

        ImGui::Separator();
        ImGui::Text("Jump to:");
        if (ImGui::Button("Dawn  (5:00)"))  tod.setHour(5.0f);  ImGui::SameLine();
        if (ImGui::Button("Noon (12:00)"))  tod.setHour(12.0f); ImGui::SameLine();
        if (ImGui::Button("Dusk (19:00)"))  tod.setHour(19.0f); ImGui::SameLine();
        if (ImGui::Button("Night (0:00)"))  tod.setHour(0.0f);

        ImGui::Separator();

        if (ImGui::CollapsingHeader("Keyframes")) {
            for (int i = 0; i < tod.keyframeCount(); ++i) {
                auto& k = tod.keyframe(i);
                ImGui::PushID(i);
                if (ImGui::TreeNode((void*)(intptr_t)i, "Key #%d  @  %.2fh", i, k.hour)) {
                    ImGui::DragFloat("Hour", &k.hour, 0.1f, 0.0f, 24.0f);
                    ImGui::ColorEdit3("Sky Top", &k.skyTop.x);
                    ImGui::ColorEdit3("Sky Bottom", &k.skyBottom.x);
                    ImGui::ColorEdit3("Sun Color", &k.sunColor.x);
                    ImGui::DragFloat("Sun Intensity", &k.sunIntensity, 0.01f, 0.0f, 5.0f);
                    ImGui::ColorEdit3("Ambient", &k.ambientColor.x);
                    ImGui::DragFloat("Ambient Intensity", &k.ambientIntensity, 0.01f, 0.0f, 5.0f);
                    ImGui::ColorEdit3("Fog Color", &k.fogColor.x);
                    ImGui::DragFloat("Fog Start", &k.fogStart, 1.0f, 0.0f, 1000.0f);
                    ImGui::DragFloat("Fog End", &k.fogEnd, 1.0f, 0.0f, 5000.0f);
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
        }

        ImGui::End();
    }

} // namespace grit::editor