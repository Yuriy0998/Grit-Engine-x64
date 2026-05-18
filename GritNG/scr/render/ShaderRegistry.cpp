#include "ShaderRegistry.h"
#include "../core/FileWatcher.h"
#include "../core/Log.h"
#include "../rhi/Resources.h"

#include <bgfx/bgfx.h>

namespace grit::render {

    void ShaderRegistry::shutdown() {
        for (auto& e : m_entries) {
            if (bgfx::isValid(e->program)) {
                bgfx::destroy(e->program);
                e->program = BGFX_INVALID_HANDLE;
            }
            if (m_watcher) {
                m_watcher->unwatch(rhi::shaderBinPath(e->vsName.c_str()));
                m_watcher->unwatch(rhi::shaderBinPath(e->fsName.c_str()));
            }
        }
        m_entries.clear();
    }

    void ShaderRegistry::clear() { shutdown(); }

    ShaderRegistry::Entry* ShaderRegistry::loadProgram(const std::string& vsName,
        const std::string& fsName) {
        for (auto& e : m_entries) {
            if (e->vsName == vsName && e->fsName == fsName) return e.get();
        }

        auto entry = std::make_unique<Entry>();
        entry->vsName = vsName;
        entry->fsName = fsName;
        entry->program = rhi::loadProgram(vsName.c_str(), fsName.c_str());

        if (!bgfx::isValid(entry->program)) {
            GRIT_ERROR("ShaderRegistry: failed to load %s + %s",
                vsName.c_str(), fsName.c_str());
        }

        Entry* raw = entry.get();
        m_entries.push_back(std::move(entry));

        if (m_watcher) {
            registerWatch(raw->vsName, raw);
            registerWatch(raw->fsName, raw);
        }
        return raw;
    }

    void ShaderRegistry::registerWatch(const std::string& shaderName, Entry* owner) {
        std::string path = rhi::shaderBinPath(shaderName.c_str());
        GRIT_INFO("ShaderRegistry: watching %s", path.c_str());
        m_watcher->watch(path, [this, owner](const std::string& changedPath) {
            GRIT_INFO("Shader changed: %s -> reloading (%s + %s)",
                changedPath.c_str(),
                owner->vsName.c_str(), owner->fsName.c_str());
            reloadProgram(owner);
            });
    }

    void ShaderRegistry::reloadProgram(Entry* e) {
        if (!e) return;

        bgfx::ProgramHandle newProg =
            rhi::loadProgram(e->vsName.c_str(), e->fsName.c_str());

        if (!bgfx::isValid(newProg)) {
            GRIT_ERROR("ShaderRegistry: reload failed for %s + %s",
                e->vsName.c_str(), e->fsName.c_str());
            return;
        }

        if (bgfx::isValid(e->program)) bgfx::destroy(e->program);
        e->program = newProg;
        GRIT_INFO("ShaderRegistry: reload OK (%s + %s)",
            e->vsName.c_str(), e->fsName.c_str());
    }

} // namespace grit::render