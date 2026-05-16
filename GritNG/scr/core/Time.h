#pragma once
#include <chrono>
#include "Types.h"

namespace grit {

    class Clock {
    public:
        Clock() : m_start(clk::now()), m_prev(m_start) {}

        void tick() {
            auto now = clk::now();
            m_delta = std::chrono::duration<f64>(now - m_prev).count();
            m_prev = now;
        }

        f64 deltaSeconds() const { return m_delta; }
        f64 totalSeconds() const {
            return std::chrono::duration<f64>(clk::now() - m_start).count();
        }

    private:
        using clk = std::chrono::high_resolution_clock;
        clk::time_point m_start, m_prev;
        f64 m_delta{ 0 };
    };

} // namespace grit