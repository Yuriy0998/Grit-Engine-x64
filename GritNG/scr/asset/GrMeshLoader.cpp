#include "GrMeshLoader.h"
#include "GrMeshFormat.h"
#include "../core/Log.h"

#include <cstdio>
#include <vector>
#include <cstring>

namespace grit::asset {

    using namespace grmesh;

    static bool readBytes(FILE* f, void* dst, size_t n) {
        return std::fread(dst, 1, n, f) == n;
    }

    bool loadGrMesh(const std::string& path, GrMeshData& out) {
        FILE* f = std::fopen(path.c_str(), "rb");
        if (!f) {
            GRIT_ERROR("grmesh: cannot open %s", path.c_str());
            return false;
        }

        Header hdr{};
        if (!readBytes(f, &hdr, sizeof(hdr))) {
            GRIT_ERROR("grmesh: header read failed: %s", path.c_str());
            std::fclose(f);
            return false;
        }

        if (hdr.magic != kMagic) {
            GRIT_ERROR("grmesh: bad magic in %s", path.c_str());
            std::fclose(f);
            return false;
        }
        if (hdr.version != kVersion) {
            GRIT_ERROR("grmesh: unsupported version %u (expected %u): %s",
                hdr.version, kVersion, path.c_str());
            std::fclose(f);
            return false;
        }
        if (hdr.vertexFormat != VFMT_PN_UV) {
            GRIT_ERROR("grmesh: unsupported vertex format %u: %s",
                hdr.vertexFormat, path.c_str());
            std::fclose(f);
            return false;
        }

        std::vector<VertexPNUV> verts;
        std::vector<u32>        indices32;
        std::vector<u16>        indices16;
        bool                    using32bit = false;

        while (true) {
            ChunkHeader ch{};
            if (!readBytes(f, &ch, sizeof(ch))) {
                GRIT_ERROR("grmesh: unexpected EOF in %s", path.c_str());
                std::fclose(f);
                return false;
            }

            if (ch.id == kChunkEND) {
                break;
            }

            std::vector<u8> data(ch.dataSize);
            if (ch.dataSize && !readBytes(f, data.data(), ch.dataSize)) {
                GRIT_ERROR("grmesh: chunk read failed in %s", path.c_str());
                std::fclose(f);
                return false;
            }
            // padding до 4
            u32 pad = (4 - (ch.dataSize & 3)) & 3;
            if (pad) std::fseek(f, pad, SEEK_CUR);

            switch (ch.id) {
            case kChunkVBUF: {
                if (ch.dataSize < 4) break;
                u32 vcount = *reinterpret_cast<u32*>(data.data());
                if (ch.dataSize != 4 + vcount * sizeof(VertexPNUV)) {
                    GRIT_ERROR("grmesh: VBUF size mismatch");
                    std::fclose(f);
                    return false;
                }
                verts.resize(vcount);
                std::memcpy(verts.data(), data.data() + 4, vcount * sizeof(VertexPNUV));
                break;
            }

            case kChunkIBUF: {
                if (ch.dataSize < 8) break;
                u32 icount = *reinterpret_cast<u32*>(data.data() + 0);
                u32 indexSize = *reinterpret_cast<u32*>(data.data() + 4);
                const u8* src = data.data() + 8;

                if (indexSize == 4) {
                    using32bit = true;
                    indices32.resize(icount);
                    std::memcpy(indices32.data(), src, icount * 4);
                }
                else if (indexSize == 2) {
                    using32bit = false;
                    indices16.resize(icount);
                    std::memcpy(indices16.data(), src, icount * 2);
                }
                else {
                    GRIT_ERROR("grmesh: unsupported indexSize=%u", indexSize);
                    std::fclose(f);
                    return false;
                }
                break;
            }

            case kChunkSUBM: {
                if (ch.dataSize < 4) break;
                u32 count = *reinterpret_cast<u32*>(data.data());
                const SubmeshRecord* recs =
                    reinterpret_cast<const SubmeshRecord*>(data.data() + 4);
                out.submeshes.resize(count);
                for (u32 i = 0; i < count; ++i) {
                    out.submeshes[i].indexStart = recs[i].indexStart;
                    out.submeshes[i].indexCount = recs[i].indexCount;
                    out.submeshes[i].materialIndex = recs[i].materialIndex;
                }
                break;
            }

            case kChunkAABB: {
                if (ch.dataSize >= sizeof(f32) * 6) {
                    const f32* p = reinterpret_cast<const f32*>(data.data());
                    out.aabbMin = vec3{ p[0], p[1], p[2] };
                    out.aabbMax = vec3{ p[3], p[4], p[5] };
                }
                break;
            }

            case kChunkMTLS: {
                if (ch.dataSize < 4) break;
                u32 count = *reinterpret_cast<u32*>(data.data());
                const MaterialRecord* recs =
                    reinterpret_cast<const MaterialRecord*>(data.data() + 4);
                out.materials.resize(count);
                for (u32 i = 0; i < count; ++i) {
                    auto& m = out.materials[i];
                    m.name = recs[i].name;
                    m.albedoTexture = recs[i].albedoTexture;
                    std::memcpy(m.baseColor, recs[i].baseColor, sizeof(m.baseColor));
                    m.metallic = recs[i].metallic;
                    m.roughness = recs[i].roughness;
                    std::memcpy(m.emissive, recs[i].emissive, sizeof(m.emissive));
                    m.flags = recs[i].flags;
                }
                break;
            }

            case kChunkNAME: {
                if (ch.dataSize < 4) break;
                u32 len = *reinterpret_cast<u32*>(data.data());
                if (4 + len <= ch.dataSize) {
                    out.name.assign(
                        reinterpret_cast<const char*>(data.data() + 4), len);
                }
                break;
            }

            default:
                // Неизвестный chunk — игнорируем (forward compat)
                break;
            }
        }

        std::fclose(f);

        if (verts.empty()) {
            GRIT_ERROR("grmesh: no vertices in %s", path.c_str());
            return false;
        }
        if (!using32bit && indices16.empty()) {
            GRIT_ERROR("grmesh: no indices in %s", path.c_str());
            return false;
        }
        if (using32bit && indices32.empty()) {
            GRIT_ERROR("grmesh: no indices in %s", path.c_str());
            return false;
        }

        // Если submeshes не были заданы — генерируем один на весь меш
        if (out.submeshes.empty()) {
            GrMeshSubmesh s;
            s.indexStart = 0;
            s.indexCount = using32bit ? (u32)indices32.size() : (u32)indices16.size();
            s.materialIndex = 0;
            out.submeshes.push_back(s);
        }

        // Создаём StaticMesh.
        // Сейчас StaticMesh::createFromArrays принимает только u32 индексы и StaticVertex —
        // конвертируем если надо.
        std::vector<render::StaticVertex> sv(verts.size());
        for (size_t i = 0; i < verts.size(); ++i) {
            sv[i].px = verts[i].px; sv[i].py = verts[i].py; sv[i].pz = verts[i].pz;
            sv[i].nx = verts[i].nx; sv[i].ny = verts[i].ny; sv[i].nz = verts[i].nz;
            sv[i].u = verts[i].u;  sv[i].v = verts[i].v;
        }

        std::vector<u32> idx;
        if (using32bit) {
            idx = std::move(indices32);
        }
        else {
            idx.resize(indices16.size());
            for (size_t i = 0; i < indices16.size(); ++i) idx[i] = indices16[i];
        }

        auto mesh = std::make_shared<render::StaticMesh>();
        if (!mesh->createFromArrays(sv.data(), (u32)sv.size(),
            idx.data(), (u32)idx.size())) {
            GRIT_ERROR("grmesh: StaticMesh creation failed: %s", path.c_str());
            return false;
        }
        out.mesh = mesh;

        GRIT_INFO("grmesh loaded: %s (verts=%u, idx=%u, submeshes=%u, mats=%u)",
            path.c_str(),
            (u32)verts.size(),
            (u32)idx.size(),
            (u32)out.submeshes.size(),
            (u32)out.materials.size());
        return true;
    }

} // namespace grit::asset