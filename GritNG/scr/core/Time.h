#pragma once

#include "Types.h"

namespace grit {

    // Глобальные тайминги движка. Обновляется Engine в начале каждого кадра.
    class Time {
    public:
        // Время dt текущего кадра (секунды). Может быть зажат maxDt.
        f32 deltaTime() const { return m_dt; }

        // Сырое dt без зажима (для логов/отладки).
        f32 rawDeltaTime() const { return m_rawDt; }

        // Накопленное время с момента старта (секунды).
        f64 totalTime() const { return m_total; }

        // Номер текущего кадра, начиная с 0.
        u64 frameCount() const { return m_frame; }

        // Усреднённый FPS (за последнюю секунду).
        f32 fps() const { return m_fps; }

        // Усреднённое время кадра (миллисекунды).
        f32 msPerFrame() const { return m_msPerFrame; }

        // Максимальное допустимое dt (защита от «провалов» при breakpoint'ах
        // или загрузке ресурсов). По умолчанию 1/10 сек.
        void  setMaxDeltaTime(f32 v) { m_maxDt = v; }
        f32   maxDeltaTime() const { return m_maxDt; }

        // --- Только для Engine ---
        void _tick(f32 rawDt);
        void _reset();

    private:
        f32 m_rawDt = 0.f;
        f32 m_dt = 0.f;
        f64 m_total = 0.0;
        u64 m_frame = 0;
        f32 m_maxDt = 0.1f;

        // Сглаживание FPS: накапливаем кадры в окне 1 сек
        f32 m_fps = 0.f;
        f32 m_msPerFrame = 0.f;
        f32 m_accumTime = 0.f;
        u32 m_accumFrames = 0;
    };

} // namespace grit