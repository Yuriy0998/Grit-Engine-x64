#include "EditorUI.h"
#include "EditorUI_Internal.h"
#include <imgui.h>
#include <ImGuizmo.h>
#include <bx/math.h>
#include <bgfx/bgfx.h>
#include <cfloat>
#include <filesystem>

#include "../Engine.h"
#include "../ecs/World.h"
#include "../ecs/Light.h"
#include "../render/Renderer.h"
#include "../render/Renderer_Internal.h"
#include "../render/StaticMesh.h"
#include "../render/Material.h"
#include "../core/Log.h"
#include "../scene/PrefabSerializer.h"
#include "Picking.h"

namespace grit::editor {

    using namespace grit::editor::detail;

    void EditorUI::drawCenterArea()
    {
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        const float bottomOffset = ImGui::GetFrameHeight();

        ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + kLeftBarW,
            vp->WorkPos.y + kTopOffset));
        ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x - kLeftBarW,
            vp->WorkSize.y - kTopOffset - bottomOffset));

        ImGuiWindowFlags fl = ImGuiWindowFlags_NoDecoration
            | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("##Viewport", nullptr, fl);

        switch (m_centerTab) {
        case CenterTab::MapEditor: {
            const ImVec2 avail = ImGui::GetContentRegionAvail();
            const uint32_t w = (uint32_t)bx::max(1.0f, avail.x);
            const uint32_t h = (uint32_t)bx::max(1.0f, avail.y);

            if (w != m_lastVpW || h != m_lastVpH) {
                m_engine->renderer().resizeViewport(w, h);
                m_lastVpW = w; m_lastVpH = h;
            }

            m_vpScreenPos = ImGui::GetCursorScreenPos();
            m_vpScreenSize = avail;
            m_vpHovered = false;

            auto tex = m_engine->renderer().viewportColorTexture();
            if (bgfx::isValid(tex)) {
                const bool obl = bgfx::getCaps()->originBottomLeft;
                ImVec2 uv0 = obl ? ImVec2(0, 1) : ImVec2(0, 0);
                ImVec2 uv1 = obl ? ImVec2(1, 0) : ImVec2(1, 1);
                ImGui::Image((ImTextureID)(uintptr_t)tex.idx, avail, uv0, uv1);

                // hover именно над изображением вьюпорта
                m_vpHovered = ImGui::IsItemHovered();

                // Picking по ЛКМ
                if (m_vpHovered && !m_cameraCaptured
                    && !ImGuizmo::IsOver() && !ImGuizmo::IsUsing()
                    && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    ImVec2 mouse = ImGui::GetMousePos();
                    pickEntity(mouse.x - m_vpScreenPos.x, mouse.y - m_vpScreenPos.y);
                }

                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("GRIT_ASSET_PATH"))
                        handleAssetDrop(std::string((const char*)p->Data));
                    ImGui::EndDragDropTarget();
                }

                // === ЗАХВАТ КАМЕРЫ: только начинаем захват, если курсор над Image ===
                const bool rmb = ImGui::IsMouseDown(ImGuiMouseButton_Right);

                // Начать захват — ТОЛЬКО если ПКМ нажат именно над изображением вьюпорта
                if (!m_cameraCaptured
                    && m_vpHovered
                    && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    m_cameraCaptured = true;
                }

                // Отпустили ПКМ — снимаем захват
                if (m_cameraCaptured && !rmb) {
                    m_cameraCaptured = false;
                }
            }
            drawGizmo();
            break;
        }
        case CenterTab::Navigation:
            ImGui::TextDisabled("Navigation Editor - coming soon"); break;
        case CenterTab::Game:
            ImGui::TextDisabled("Game view - coming soon"); break;
        case CenterTab::LuaScript:
            ImGui::TextDisabled("Lua Script editor - coming soon"); break;
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void EditorUI::drawGizmo()
    {
        if (m_selected == entt::null || m_centerTab != CenterTab::MapEditor) return;

        auto& reg = m_engine->world().registry();
        auto* tr = reg.try_get<grit::ecs::Transform>(m_selected);
        if (!tr) return;

        auto& cam = m_engine->renderer().camera();
        float view[16], proj[16];
        cam.buildView(view);
        cam.buildProj(proj, true);

        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
        ImGuizmo::SetRect(m_vpScreenPos.x, m_vpScreenPos.y,
            m_vpScreenSize.x, m_vpScreenSize.y);

        float matrix[16];
        float t[3] = { tr->position.x, tr->position.y, tr->position.z };
        float r[3] = { tr->rotationEuler.x, tr->rotationEuler.y, tr->rotationEuler.z };
        float s[3] = { tr->scale.x, tr->scale.y, tr->scale.z };
        ImGuizmo::RecomposeMatrixFromComponents(t, r, s, matrix);

        ImGuizmo::OPERATION op;
        switch (m_gizmoOp) {
        case GizmoOp::Rotate: op = ImGuizmo::ROTATE; break;
        case GizmoOp::Scale:  op = ImGuizmo::SCALE;  break;
        default:              op = ImGuizmo::TRANSLATE; break;
        }
        ImGuizmo::MODE mode = m_gizmoUseLocal ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

        const bool useSnap = ImGui::GetIO().KeyCtrl;
        float snapT[3] = { 0.5f, 0.5f, 0.5f };
        float snapR[3] = { 15.0f, 15.0f, 15.0f };
        float snapS[3] = { 0.1f, 0.1f, 0.1f };
        const float* snap = nullptr;
        if (useSnap) {
            if (op == ImGuizmo::TRANSLATE) snap = snapT;
            else if (op == ImGuizmo::ROTATE)    snap = snapR;
            else                                snap = snapS;
        }

        if (ImGuizmo::Manipulate(view, proj, op, mode, matrix, nullptr, snap)) {
            float nt[3], nr[3], ns[3];
            ImGuizmo::DecomposeMatrixToComponents(matrix, nt, nr, ns);
            if (op == ImGuizmo::TRANSLATE) tr->position = { nt[0], nt[1], nt[2] };
            else if (op == ImGuizmo::ROTATE)    tr->rotationEuler = { nr[0], nr[1], nr[2] };
            else if (op == ImGuizmo::SCALE)     tr->scale = { ns[0], ns[1], ns[2] };
        }
    }

    void EditorUI::pickEntity(float mouseInVpX, float mouseInVpY)
    {
        auto& reg = m_engine->world().registry();
        auto& cam = m_engine->renderer().camera();

        float view[16], proj[16];
        cam.buildView(view);
        cam.buildProj(proj, true);

        Ray ray = screenToRay(mouseInVpX, mouseInVpY,
            m_vpScreenSize.x, m_vpScreenSize.y, view, proj);

        entt::entity best = entt::null;
        float bestT = FLT_MAX;

        // --- Меши: ray vs AABB ---
        auto vw = reg.view<grit::ecs::Transform, grit::ecs::MeshRenderer>();
        for (auto e : vw) {
            const auto& tr = vw.get<grit::ecs::Transform>(e);
            const auto& mr = vw.get<grit::ecs::MeshRenderer>(e);
            if (!mr.visible || !mr.mesh) continue;

            float mtx[16];
            bx::mtxSRT(mtx,
                tr.scale.x, tr.scale.y, tr.scale.z,
                tr.rotationEuler.x, tr.rotationEuler.y, tr.rotationEuler.z,
                tr.position.x, tr.position.y, tr.position.z);

            vec3 wmn, wmx;
            transformAABB(mtx, mr.mesh->aabbMin(), mr.mesh->aabbMax(), wmn, wmx);

            float t;
            if (rayIntersectAABB(ray, wmn, wmx, t) && t < bestT) {
                bestT = t;
                best = e;
            }
        }

        // --- Lights: ray vs sphere ---
        constexpr float kLightPickRadiusSq = 0.35f * 0.35f;

        auto tryPickLight = [&](entt::entity ent, const vec3& pos) {
            vec3 op{ pos.x - ray.origin.x, pos.y - ray.origin.y, pos.z - ray.origin.z };
            float t = op.x * ray.dir.x + op.y * ray.dir.y + op.z * ray.dir.z;
            if (t < 0.f) return;
            vec3 closest{
                ray.origin.x + ray.dir.x * t,
                ray.origin.y + ray.dir.y * t,
                ray.origin.z + ray.dir.z * t
            };
            vec3 d{ closest.x - pos.x, closest.y - pos.y, closest.z - pos.z };
            float dsq = d.x * d.x + d.y * d.y + d.z * d.z;
            if (dsq <= kLightPickRadiusSq && t < bestT) {
                bestT = t;
                best = ent;
            }
            };

        for (auto ent : reg.view<grit::ecs::LightDirectional>()) {
            vec3 p{ 0, 0, 0 };
            if (auto* tr = reg.try_get<grit::ecs::Transform>(ent)) p = tr->position;
            tryPickLight(ent, p);
        }
        for (auto ent : reg.view<grit::ecs::Transform, grit::ecs::LightPoint>()) {
            tryPickLight(ent, reg.get<grit::ecs::Transform>(ent).position);
        }
        for (auto ent : reg.view<grit::ecs::Transform, grit::ecs::LightSpot>()) {
            tryPickLight(ent, reg.get<grit::ecs::Transform>(ent).position);
        }

        m_selected = best;
    }

    void EditorUI::handleAssetDrop(const std::string& path)
    {
        namespace fs = std::filesystem;
        fs::path p(path);
        const std::string ext = p.extension().string();
        const std::string name = p.stem().string();
        auto prog = m_engine->renderer().defaultProgram();

        if (ext == ".grmesh") {
            auto& assets = m_engine->assets();
            auto model = assets.loadModel(path, prog);
            if (!model.mesh) {
                GRIT_WARN("Drop: failed to load mesh '%s'", path.c_str());
                return;
            }

            auto& reg = m_engine->world().registry();
            auto e = reg.create();
            reg.emplace<grit::ecs::Name>(e, grit::ecs::Name{ name });
            reg.emplace<grit::ecs::Transform>(e);

            grit::ecs::MeshRenderer mr;
            mr.mesh = model.mesh;
            mr.meshPath = path;
            mr.visible = true;
            for (auto& m : model.materials) mr.materials.push_back(m);

            if (mr.materials.empty())
                fillDefaultMaterials(mr, prog);   // ← хелпер

            reg.emplace<grit::ecs::MeshRenderer>(e, std::move(mr));
            m_selected = e;
            GRIT_INFO("Drop: spawned entity from '%s'", path.c_str());
            return;
        }

        if (ext == ".grprefab") {
            auto e = grit::scene::PrefabSerializer::loadPrefab(
                m_engine->world(), path, m_engine->assets(), prog);
            if (e != entt::null) {
                m_selected = e;
                GRIT_INFO("Prefab spawned: %s", path.c_str());
            }
            return;
        }

        GRIT_WARN("Drop: unsupported asset type '%s'", ext.c_str());
    }

} // namespace grit::editor