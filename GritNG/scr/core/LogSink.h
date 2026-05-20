#pragma once
#include <string>
#include <vector>
#include <mutex>

namespace grit {

    enum class LogLevel { Info, Warn, Error };

    struct LogEntry {
        LogLevel    level;
        std::string text;
    };

    class LogSink {
    public:
        static LogSink& get() { static LogSink s; return s; }

        void push(LogLevel lvl, std::string msg) {
            std::lock_guard lk(m_mtx);
            if (m_entries.size() >= m_capacity) {
                // отрезаем четверть старых, чтобы не елозить каждый раз
                m_entries.erase(m_entries.begin(),
                    m_entries.begin() + (m_capacity / 4));
            }
            m_entries.push_back({ lvl, std::move(msg) });
        }

        std::vector<LogEntry> snapshot() const {
            std::lock_guard lk(m_mtx);
            return m_entries;
        }

        void clear() {
            std::lock_guard lk(m_mtx);
            m_entries.clear();
        }

    private:
        mutable std::mutex     m_mtx;
        std::vector<LogEntry>  m_entries;
        size_t                 m_capacity = 4096;
    };

} // namespace grit