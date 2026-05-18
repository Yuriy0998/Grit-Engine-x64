#pragma once

#include "Types.h"
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace grit {

    // Простой polling-watcher: периодически опрашивает mtime файлов.
    // Если файл изменился — вызывает колбэк.
    class FileWatcher {
    public:
        using Callback = std::function<void(const std::string& path)>;

        // pollIntervalSec — как часто проверять (0.25 сек по умолчанию)
        void setPollInterval(f32 sec) { m_pollInterval = sec; }

        // Добавить файл под наблюдение. Первый замер mtime запоминается, колбэк
        // НЕ вызывается на старте.
        void watch(const std::string& path, Callback cb);

        // Перестать следить за файлом
        void unwatch(const std::string& path);

        // Вызывать каждый кадр. Сам решает, опрашивать или ждать.
        void update(f32 dt);

        // Принудительная проверка прямо сейчас
        void pollNow();

        void clear() { m_entries.clear(); }

    private:
        struct Entry {
            std::filesystem::file_time_type mtime;
            Callback                        cb;
            bool                            exists = false;
        };

        std::unordered_map<std::string, Entry> m_entries;
        f32 m_accum = 0.f;
        f32 m_pollInterval = 0.25f;
    };

} // namespace grit