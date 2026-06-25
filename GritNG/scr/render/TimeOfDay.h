#pragma once
#include "Environment.h"
#include "../rhi/Resources.h"
#include "../core/Math.h"     // ← здесь grit::vec3

namespace grit::render {

    // Один ключ — параметры неба и освещения в определённый час
    struct TimeKeyframe {
        f32        hour;
        grit::vec3 skyTop;
        grit::vec3 skyBottom;
        grit::vec3 sunColor;
        f32        sunIntensity;
        grit::vec3 ambientColor;
        f32        ambientIntensity;
        grit::vec3 fogColor;
        f32        fogStart;
        f32        fogEnd;
    };

    class TimeOfDay {
    public:
        TimeOfDay();

        // Текущее время суток в часах (0..24, может крутиться циклически)
        void  setHour(f32 hour);
        f32   hour() const { return m_hour; }

        // Скорость хода времени (часы в секунду игрового времени).
        // 0 — пауза. По умолчанию 0 — управление вручную/ползунком.
        void  setSpeed(f32 hoursPerSecond) { m_speed = hoursPerSecond; }
        f32   speed() const { return m_speed; }

        void  update(f32 dt);

        // Применить интерполированные значения к Environment
        void  apply(Environment& env) const;

        // Направление солнца для текущего часа (юнит-вектор от мира к солнцу,
        // т.е. для шейдера сделать -sunDir чтобы получить направление лучей)
        grit::vec3 computeSunDirection() const;

        // Доступ к ключам для редактирования
        TimeKeyframe& keyframe(int i) { return m_keys[i]; }
        const TimeKeyframe& keyframe(int i) const { return m_keys[i]; }
        int  keyframeCount() const { return (int)m_keys.size(); }

    private:
        f32                       m_hour = 12.0f;
        f32                       m_speed = 0.0f;
        std::vector<TimeKeyframe> m_keys;

        TimeKeyframe interpolate(f32 hour) const;
    };

} // namespace grit::render