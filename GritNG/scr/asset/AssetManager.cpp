#include "AssetManager.h"
#include "GrMeshLoader.h"
#include "../render/Texture.h"
#include "../render/StaticMesh.h"
#include "../core/Log.h"

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

    void AssetManager::clear() {
        m_textures.clear();
        m_grmeshes.clear();
    }

} // namespace grit::asset