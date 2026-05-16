#pragma once
#include "../render/StaticMesh.h"
#include <memory>
#include <string>
#include <vector>

namespace grit::asset {

    struct GrMeshSubmesh {
        u32 indexStart;
        u32 indexCount;
        u32 materialIndex;
    };

    struct GrMeshMaterialDesc {
        std::string name;
        std::string albedoTexture;
        f32  baseColor[4] = { 1,1,1,1 };
        f32  metallic = 0.0f;
        f32  roughness = 1.0f;
        f32  emissive[3] = { 0,0,0 };
        u32  flags = 0;
    };

    struct GrMeshData {
        std::shared_ptr<render::StaticMesh> mesh;
        std::vector<GrMeshSubmesh>          submeshes;
        std::vector<GrMeshMaterialDesc>     materials;
        std::string                         name;
        vec3                                aabbMin{ 0,0,0 };
        vec3                                aabbMax{ 0,0,0 };
    };

    bool loadGrMesh(const std::string& path, GrMeshData& out);

} // namespace grit::asset