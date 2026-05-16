#pragma once
#include "../core/Types.h"

namespace grit::asset::grmesh {

    // FOURCC helper
    constexpr u32 fourcc(char a, char b, char c, char d) {
        return (u32(u8(a)))
            | (u32(u8(b)) << 8)
            | (u32(u8(c)) << 16)
            | (u32(u8(d)) << 24);
    }

    constexpr u32 kMagic = fourcc('G', 'R', 'M', 'S');
    constexpr u32 kVersion = 1;

    constexpr u32 kChunkVBUF = fourcc('V', 'B', 'U', 'F');
    constexpr u32 kChunkIBUF = fourcc('I', 'B', 'U', 'F');
    constexpr u32 kChunkSUBM = fourcc('S', 'U', 'B', 'M');
    constexpr u32 kChunkAABB = fourcc('A', 'A', 'B', 'B');
    constexpr u32 kChunkMTLS = fourcc('M', 'T', 'L', 'S');
    constexpr u32 kChunkNAME = fourcc('N', 'A', 'M', 'E');
    constexpr u32 kChunkEND = fourcc('E', 'N', 'D', ' ');

    enum VertexFormat : u32 {
        VFMT_PN_UV = 1,
        VFMT_PN_UV_TANG = 2,
        VFMT_PN_UV_SKIN = 3,
    };

#pragma pack(push, 1)

    struct Header {
        u32 magic;
        u32 version;
        u32 flags;
        u32 submeshCount;
        u32 vertexFormat;
        u32 reserved;
    };
    static_assert(sizeof(Header) == 24, "Header must be 24 bytes");

    struct ChunkHeader {
        u32 id;
        u32 dataSize;
    };
    static_assert(sizeof(ChunkHeader) == 8, "ChunkHeader must be 8 bytes");

    struct SubmeshRecord {
        u32 indexStart;
        u32 indexCount;
        u32 materialIndex;
    };

    struct MaterialRecord {
        char  name[64];
        char  albedoTexture[128];
        f32   baseColor[4];
        f32   metallic;
        f32   roughness;
        f32   emissive[3];
        u32   flags;
        char  _reserved[24];
    };
    static_assert(sizeof(MaterialRecord) == 256, "MaterialRecord must be 256 bytes");

    // Vertex для VFMT_PN_UV
    struct VertexPNUV {
        f32 px, py, pz;
        f32 nx, ny, nz;
        f32 u, v;
    };
    static_assert(sizeof(VertexPNUV) == 32, "VertexPNUV must be 32 bytes");

#pragma pack(pop)

} // namespace grit::asset::grmesh