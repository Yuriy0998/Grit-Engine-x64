#include "Picking.h"
#include <bx/math.h>
#include <cfloat>
#include <cmath>
#include <algorithm>

namespace grit::editor {

    Ray screenToRay(float mouseX, float mouseY,
        float vpW, float vpH,
        const float view[16],
        const float proj[16])
    {
        float x = (2.0f * mouseX / vpW) - 1.0f;
        float y = 1.0f - (2.0f * mouseY / vpH);

        float viewProj[16];
        bx::mtxMul(viewProj, view, proj);
        float invVP[16];
        bx::mtxInverse(invVP, viewProj);

        auto unproject = [&](float zNDC) {
            float in[4] = { x, y, zNDC, 1.0f };
            float out[4] = {
                in[0] * invVP[0] + in[1] * invVP[4] + in[2] * invVP[8] + in[3] * invVP[12],
                in[0] * invVP[1] + in[1] * invVP[5] + in[2] * invVP[9] + in[3] * invVP[13],
                in[0] * invVP[2] + in[1] * invVP[6] + in[2] * invVP[10] + in[3] * invVP[14],
                in[0] * invVP[3] + in[1] * invVP[7] + in[2] * invVP[11] + in[3] * invVP[15],
            };
            if (std::abs(out[3]) > 1e-8f) {
                out[0] /= out[3];
                out[1] /= out[3];
                out[2] /= out[3];
            }
            return vec3{ out[0], out[1], out[2] };
            };

        vec3 nearP = unproject(-1.0f);
        vec3 farP = unproject(1.0f);

        vec3 dir{
            farP.x - nearP.x,
            farP.y - nearP.y,
            farP.z - nearP.z,
        };
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        if (len > 1e-6f) { dir.x /= len; dir.y /= len; dir.z /= len; }

        return { nearP, dir };
    }

    bool rayIntersectAABB(const Ray& ray,
        const vec3& mn,
        const vec3& mx,
        float& tOut)
    {
        float tmin = -FLT_MAX;
        float tmax = FLT_MAX;

        const float o[3] = { ray.origin.x, ray.origin.y, ray.origin.z };
        const float d[3] = { ray.dir.x,    ray.dir.y,    ray.dir.z };
        const float a[3] = { mn.x, mn.y, mn.z };
        const float b[3] = { mx.x, mx.y, mx.z };

        for (int i = 0; i < 3; ++i) {
            if (std::abs(d[i]) < 1e-6f) {
                if (o[i] < a[i] || o[i] > b[i]) return false;
            }
            else {
                float inv = 1.0f / d[i];
                float t1 = (a[i] - o[i]) * inv;
                float t2 = (b[i] - o[i]) * inv;
                if (t1 > t2) std::swap(t1, t2);
                tmin = std::max(tmin, t1);
                tmax = std::min(tmax, t2);
                if (tmax < tmin) return false;
            }
        }
        if (tmax < 0.0f) return false;
        tOut = (tmin >= 0.0f) ? tmin : tmax;
        return true;
    }

    void transformAABB(const float m[16],
        const vec3& inMin, const vec3& inMax,
        vec3& outMin, vec3& outMax)
    {
        const float c[8][3] = {
            { inMin.x, inMin.y, inMin.z },
            { inMax.x, inMin.y, inMin.z },
            { inMin.x, inMax.y, inMin.z },
            { inMax.x, inMax.y, inMin.z },
            { inMin.x, inMin.y, inMax.z },
            { inMax.x, inMin.y, inMax.z },
            { inMin.x, inMax.y, inMax.z },
            { inMax.x, inMax.y, inMax.z },
        };

        vec3 mn{ FLT_MAX,  FLT_MAX,  FLT_MAX };
        vec3 mx{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

        for (int i = 0; i < 8; ++i) {
            float x = c[i][0], y = c[i][1], z = c[i][2];
            float wx = x * m[0] + y * m[4] + z * m[8] + m[12];
            float wy = x * m[1] + y * m[5] + z * m[9] + m[13];
            float wz = x * m[2] + y * m[6] + z * m[10] + m[14];
            mn.x = std::min(mn.x, wx);  mx.x = std::max(mx.x, wx);
            mn.y = std::min(mn.y, wy);  mx.y = std::max(mx.y, wy);
            mn.z = std::min(mn.z, wz);  mx.z = std::max(mx.z, wz);
        }
        outMin = mn;
        outMax = mx;
    }

} // namespace grit::editor