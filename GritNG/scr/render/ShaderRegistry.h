#pragma once

#include "../core/Types.h"
#include <bgfx/bgfx.h>
#include <string>
#include <vector>
#include <memory>

namespace grit {
    class FileWatcher;
}

namespace grit::render {

    // Регистрирует bgfx-программы по парам (vs, fs) и обеспечивает hot reload
    // при изменении соответствующих .bin файлов на диске.
    class ShaderRegistry {
    public:
        // Идентификатор зарегистрированной программы. Используй только через registry.
        struct Entry {
            std::string         vsName;
            std::string         fsName;
            bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;
        };

        // Привязать к watcher. Можно nullptr — тогда reload не будет.
        void init(FileWatcher* watcher) { m_watcher = watcher; }
        void shutdown();

        // Загрузить программу. Если такая пара уже есть — вернёт существующую.
        // Возвращает указатель на Entry (стабильный, валидный до shutdown/clear).
        // Через Entry::program читай актуальный handle прямо перед submit.
        Entry* loadProgram(const std::string& vsName, const std::string& fsName);

        // Принудительно перезагрузить программу (обычно вызывается из watcher колбэка)
        void reloadProgram(Entry* e);

        void clear();

    private:
        void registerWatch(const std::string& shaderName, Entry* owner);

        FileWatcher* m_watcher = nullptr;
        std::vector<std::unique_ptr<Entry>>  m_entries;
    };

} // namespace grit::render