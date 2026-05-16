#pragma once
#include "Types.h"
#include <cmath>

namespace grit {

    struct vec3 {
        f32 x{ 0 }, y{ 0 }, z{ 0 };
        vec3() = default;
        vec3(f32 v) : x(v), y(v), z(v) {}
        vec3(f32 _x, f32 _y, f32 _z) : x(_x), y(_y), z(_z) {}
    };

    struct vec4 { f32 x{}, y{}, z{}, w{}; };

    struct mat4 {
        // column-major (как bgfx/HLSL по умолчанию для умножения mul(M,v))
        f32 m[16]{};

        static mat4 identity() {
            mat4 r;
            r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
            return r;
        }
    };

    inline vec3 operator+(vec3 a, vec3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
    inline vec3 operator-(vec3 a, vec3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
    inline vec3 operator*(vec3 a, f32 s) { return { a.x * s, a.y * s, a.z * s }; }
    inline vec3 operator-(vec3 a) { return { -a.x, -a.y, -a.z }; }

    inline f32  dot(vec3 a, vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
    inline vec3 cross(vec3 a, vec3 b) {
        return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
    }
    inline f32  length(vec3 a) { return std::sqrt(dot(a, a)); }
    inline vec3 normalize(vec3 a) {
        f32 l = length(a);
        return (l > 1e-8f) ? a * (1.0f / l) : vec3{ 0,0,0 };
    }

    constexpr f32 PI = 3.14159265358979323846f;
    inline f32 radians(f32 deg) { return deg * (PI / 180.0f); }

} // namespace grit