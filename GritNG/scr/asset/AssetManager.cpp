#include "AssetManager.h"
#include "GrMeshLoader.h"
#include "../render/Texture.h"
#include "../render/StaticMesh.h"
#include "../core/Log.h"
#include "../render/Material.h"

namespace grit::asset {

    AssetManager::AssetManager() = default;
    AssetManager::~AssetManager() { clear(); }

    AssetHandle<render::Texture> AssetManager::loadTexture(const std::string& path) {
        auto it = m_textures.find(path);
        if (it != m_textures.end()) {
            return AssetHandle<render::Texture>(it->second);
        }

        auto tex = std::make_shared<render::Texture>();
        if (!tex->loadFromFile(path.c_str())) {
            return {};
        }
        m_textures.emplace(path, tex);
        return AssetHandle<render::Texture>(tex);
    }

    AssetHandle<render::StaticMesh> AssetManager::loadStaticMesh(const std::string& path) {
        const GrMeshData* data = loadGrMesh(path);
        if (!data || !data->mesh) return {};
        return AssetHandle<render::StaticMesh>(data->mesh);
    }

    const GrMeshData* AssetManager::loadGrMesh(const std::string& path) {
        auto it = m_grmeshes.find(path);
        if (it != m_grmeshes.end()) {
            return &it->second;
        }

        GrMeshData data;
        if (!grit::asset::loadGrMesh(path, data)) {  // вызов свободной функции
            return nullptr;
        }
        auto [ins, _] = m_grmeshes.emplace(path, std::move(data));
        return &ins->second;
    }


    std::shared_ptr<render::Material> AssetManager::createMaterial(
        const GrMeshMaterialDesc& desc,
        bgfx::ProgramHandle       program,
        const std::string& textureBaseDir)
    {
        auto mat = std::make_shared<render::Material>();
        mat->program = program;

        // Параметры
        mat->baseColor = vec4{
            desc.baseColor[0], desc.baseColor[1],
            desc.baseColor[2], desc.baseColor[3]
        };
        mat->tilingOffset = vec4{ 1.0f, 1.0f, 0.0f, 0.0f };
        mat->pbrParams = vec4{ desc.metallic, desc.roughness, 0.0f, 0.0f };
        mat->emissive = vec4{
            desc.emissive[0], desc.emissive[1], desc.emissive[2], 1.0f
        };

        // Albedo текстура (опционально)
        if (!desc.albedoTexture.empty()) {
            // Если в .grmesh уже абсолютный/полный путь — используем как есть.
            // Иначе считаем относительно textureBaseDir.
            std::string path = desc.albedoTexture;
            const bool looksAbsolute =
                (path.size() >= 2 && (path[1] == ':' || path[0] == '/'))
                || path.find('/') != std::string::npos
                || path.find('\\') != std::string::npos;

            if (!looksAbsolute) {
                path = textureBaseDir + desc.albedoTexture;
            }

            auto tex = loadTexture(path);
            if (tex) {
                mat->albedo = tex;
            }
            else {
                GRIT_WARN("createMaterial: failed to load albedo '%s'", path.c_str());
            }
        }

        return mat;
    }


    AssetManager::LoadedModel AssetManager::loadModel(
        const std::string& path,
        bgfx::ProgramHandle program,
        const std::string& textureBaseDir)
    {
        LoadedModel result;

        const GrMeshData* data = loadGrMesh(path);
        if (!data || !data->mesh) {
            return result;
        }
        result.mesh = data->mesh;

        result.materials.reserve(data->materials.size());
        for (const auto& desc : data->materials) {
            result.materials.push_back(createMaterial(desc, program, textureBaseDir));
        }

        return result;
    }


    void AssetManager::clear() {
        m_textures.clear();
        m_grmeshes.clear();
    }

} // namespace grit::asset