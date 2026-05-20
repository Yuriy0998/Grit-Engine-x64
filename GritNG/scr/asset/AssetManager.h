#pragma once
#include "AssetHandle.h"
#include "GrMeshLoader.h"
#include <bgfx/bgfx.h>
#include <string>
#include <unordered_map>
#include <memory>

namespace grit::render { class Texture; class StaticMesh; class Material; }

namespace grit::asset {

    class AssetManager {
    public:
        AssetManager();
        ~AssetManager();

        AssetHandle<render::Texture>    loadTexture(const std::string& path);
        AssetHandle<render::StaticMesh> loadStaticMesh(const std::string& path);
        const GrMeshData* loadGrMesh(const std::string& path);

        // Создаёт материал по описанию из .grmesh.
        // - baseColor, metallic, roughness, emissive берутся из GrMeshMaterialDesc
        // - albedoTexture (если есть) грузится через loadTexture
        // - program передаётся снаружи (как правило defaultProgram у Renderer'а)
        // - textureBaseDir используется как префикс для относительных путей текстур,
        //   например "assets/textures/"
        std::shared_ptr<render::Material> createMaterial(
            const GrMeshMaterialDesc& desc,
            bgfx::ProgramHandle       program,
            const std::string& textureBaseDir = "assets/textures/");

        void clear();

        struct LoadedModel {
            std::shared_ptr<render::StaticMesh>            mesh;
            std::vector<std::shared_ptr<render::Material>> materials;
        };

        LoadedModel loadModel(const std::string& path,
            bgfx::ProgramHandle program,
            const std::string& textureBaseDir = "assets/textures/");

    private:
        std::unordered_map<std::string, std::shared_ptr<render::Texture>> m_textures;
        std::unordered_map<std::string, GrMeshData>                       m_grmeshes;
    };

} // namespace grit::asset