#include "FileWatcher.h"
#include "Log.h"

#include <system_error>

namespace fs = std::filesystem;

namespace grit {

    void FileWatcher::watch(const std::string& path, Callback cb) {
        Entry e;
        e.cb = std::move(cb);
        std::error_code ec;
        if (fs::exists(path, ec)) {
            e.mtime = fs::last_write_time(path, ec);
            e.exists = !ec;
        }
        m_entries[path] = std::move(e);
    }

    void FileWatcher::unwatch(const std::string& path) {
        m_entries.erase(path);
    }

    void FileWatcher::update(f32 dt) {
        m_accum += dt;
        if (m_accum < m_pollInterval) return;
        m_accum = 0.f;
        pollNow();
    }

    void FileWatcher::pollNow() {
        std::error_code ec;
        for (auto& [path, e] : m_entries) {
            if (!fs::exists(path, ec)) {
                e.exists = false;
                continue;
            }
            auto newMtime = fs::last_write_time(path, ec);
            if (ec) continue;

            if (!e.exists) {
                // ‘айл по€вилс€ Ч считаем за изменение
                e.exists = true;
                e.mtime = newMtime;
                if (e.cb) e.cb(path);
                continue;
            }

            if (newMtime != e.mtime) {
                e.mtime = newMtime;
                if (e.cb) {
                    try {
                        e.cb(path);
                    }
                    catch (...) {
                        GRIT_ERROR("Shader file not found: %s", path);
                    }
                }
            }
        }
    }

} // namespace grit