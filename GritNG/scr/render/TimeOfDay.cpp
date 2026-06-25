#include "TimeOfDay.h"
#include <bx/math.h>
#include <algorithm>
#include <cmath>

namespace grit::render {

    static grit::vec3 lerp3(const grit::vec3& a, const grit::vec3& b, f32 t) {
        return {
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t
        };
    }

    TimeOfDay::TimeOfDay() {
        m_keys = {
            { 0.0f,  {0.02f,0.03f,0.08f}, {0.05f,0.06f,0.12f}, {0.20f,0.25f,0.45f}, 0.10f, {0.08f,0.10f,0.18f}, 0.6f, {0.05f,0.07f,0.12f}, 20.f, 180.f },
            { 5.0f,  {0.10f,0.12f,0.25f}, {0.40f,0.30f,0.30f}, {0.80f,0.50f,0.30f}, 0.40f, {0.20f,0.18f,0.22f}, 0.8f, {0.45f,0.35f,0.35f}, 25.f, 200.f },
            { 7.0f,  {0.30f,0.55f,0.85f}, {0.95f,0.75f,0.55f}, {1.00f,0.85f,0.65f}, 0.90f, {0.40f,0.42f,0.50f}, 1.0f, {0.78f,0.78f,0.80f}, 40.f, 280.f },
            {12.0f,  {0.25f,0.50f,0.95f}, {0.80f,0.90f,1.00f}, {1.00f,0.97f,0.90f}, 1.30f, {0.55f,0.58f,0.65f}, 1.0f, {0.80f,0.85f,0.92f}, 60.f, 400.f },
            {17.0f,  {0.30f,0.55f,0.85f}, {0.95f,0.80f,0.55f}, {1.00f,0.85f,0.60f}, 1.00f, {0.50f,0.45f,0.45f}, 1.0f, {0.80f,0.72f,0.62f}, 40.f, 280.f },
            {19.0f,  {0.20f,0.20f,0.40f}, {0.95f,0.45f,0.25f}, {1.00f,0.55f,0.25f}, 0.50f, {0.30f,0.22f,0.25f}, 0.9f, {0.55f,0.35f,0.30f}, 25.f, 200.f },
            {21.0f,  {0.05f,0.07f,0.18f}, {0.10f,0.12f,0.22f}, {0.30f,0.35f,0.55f}, 0.15f, {0.10f,0.12f,0.20f}, 0.7f, {0.08f,0.10f,0.18f}, 20.f, 180.f },
            {24.0f,  {0.02f,0.03f,0.08f}, {0.05f,0.06f,0.12f}, {0.20f,0.25f,0.45f}, 0.10f, {0.08f,0.10f,0.18f}, 0.6f, {0.05f,0.07f,0.12f}, 20.f, 180.f },
        };
    }

    void TimeOfDay::setHour(f32 hour) {
        m_hour = bx::mod(hour, 24.0f);
        if (m_hour < 0.0f) m_hour += 24.0f;
    }

    void TimeOfDay::update(f32 dt) {
        if (m_speed <= 0.0001f) return;
        setHour(m_hour + m_speed * dt);
    }

    TimeKeyframe TimeOfDay::interpolate(f32 hour) const {
        const int n = (int)m_keys.size();
        for (int i = 0; i < n - 1; ++i) {
            const auto& a = m_keys[i];
            const auto& b = m_keys[i + 1];
            if (hour >= a.hour && hour <= b.hour) {
                f32 t = (hour - a.hour) / std::max(b.hour - a.hour, 0.0001f);
                TimeKeyframe r;
                r.hour = hour;
                r.skyTop = lerp3(a.skyTop, b.skyTop, t);
                r.skyBottom = lerp3(a.skyBottom, b.skyBottom, t);
                r.sunColor = lerp3(a.sunColor, b.sunColor, t);
                r.sunIntensity = a.sunIntensity + (b.sunIntensity - a.sunIntensity) * t;
                r.ambientColor = lerp3(a.ambientColor, b.ambientColor, t);
                r.ambientIntensity = a.ambientIntensity + (b.ambientIntensity - a.ambientIntensity) * t;
                r.fogColor = lerp3(a.fogColor, b.fogColor, t);
                r.fogStart = a.fogStart + (b.fogStart - a.fogStart) * t;
                r.fogEnd = a.fogEnd + (b.fogEnd - a.fogEnd) * t;
                return r;
            }
        }
        return m_keys.back();
    }

    grit::vec3 TimeOfDay::computeSunDirection() const {
        // 6:00 — солнце на горизонте (восток), 12:00 — зенит, 18:00 — закат (запад)
        f32 t = (m_hour - 6.0f) / 12.0f;     // 0..1 за день
        f32 a = (t - 0.5f) * bx::kPi;        // -pi/2 .. +pi/2

        f32 cy = bx::sin(a);                 // высота
        f32 cx = bx::cos(a);                 // запад-восток
        f32 cz = 0.0f;

        grit::vec3 toSun = { cx, cy, cz };
        f32 len = std::sqrt(toSun.x * toSun.x + toSun.y * toSun.y + toSun.z * toSun.z);
        if (len < 0.0001f) return { 0.0f, 1.0f, 0.0f };
        return { toSun.x / len, toSun.y / len, toSun.z / len };
    }

    void TimeOfDay::apply(Environment& env) const {
        TimeKeyframe k = interpolate(m_hour);

        env.skyTopColor = k.skyTop;
        env.skyBottomColor = k.skyBottom;
        env.sunColor = k.sunColor;
        env.sunIntensity = k.sunIntensity;
        env.ambientColor = k.ambientColor;
        env.ambientIntensity = k.ambientIntensity;
        env.fogColor = k.fogColor;
        env.fogStart = k.fogStart;
        env.fogEnd = k.fogEnd;

        grit::vec3 toSun = computeSunDirection();
        // env.sunDirection — направление ЛУЧЕЙ от солнца к миру
        env.sunDirection = grit::vec3{ -toSun.x, -toSun.y, -toSun.z };

        env.timeOfDay = m_hour;
    }

} // namespace grit::render