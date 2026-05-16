#pragma once
#include "AssetHandle.h"
#include "GrMeshLoader.h"   // GrMeshData
#include <string>
#include <unordered_map>
#include <memory>

namespace grit::render { class Texture; class StaticMesh; }

namespace grit::asset {

    class AssetManager {
    public:
        AssetManager();
        ~AssetManager();

        // Текстура: KTX/KTX2/DDS/PNG/JPG через bimg
        AssetHandle<render::Texture> loadTexture(const std::string& path);

        // .grmesh: возвращает только меш (часто этого хватает)
        AssetHandle<render::StaticMesh> loadStaticMesh(const std::string& path);

        // .grmesh со всеми metadata (submeshes, материалы, AABB, имя)
        // Также кэшируется — повторный вызов отдаст ту же запись
        const GrMeshData* loadGrMesh(const std::string& path);

        void clear();

    private:
        std::unordered_map<std::string, std::shared_ptr<render::Texture>> m_textures;
        std::unordered_map<std::string, GrMeshData>                       m_grmeshes;
    };

} // namespace grit::asset