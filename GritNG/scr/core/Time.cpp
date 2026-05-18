#include "Time.h"

namespace grit {

    void Time::_tick(f32 rawDt) {
        m_rawDt = rawDt;
        m_dt = (rawDt > m_maxDt) ? m_maxDt : rawDt;
        m_total += (f64)m_dt;
        m_frame++;

        // FPS сглаживается по окну 1 сек
        m_accumTime += m_dt;
        m_accumFrames += 1;
        if (m_accumTime >= 1.0f) {
            m_fps = (f32)m_accumFrames / m_accumTime;
            m_msPerFrame = (m_accumTime * 1000.f) / (f32)m_accumFrames;
            m_accumTime = 0.f;
            m_accumFrames = 0;
        }
    }

    void Time::_reset() {
        m_rawDt = m_dt = 0.f;
        m_total = 0.0;
        m_frame = 0;
        m_fps = m_msPerFrame = 0.f;
        m_accumTime = 0.f;
        m_accumFrames = 0;
    }

} // namespace grit