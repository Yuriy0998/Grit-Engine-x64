#pragma once
#include <cstdio>
#include <cstdarg>

namespace grit::log {

    enum class Level { Info, Warn, Error };

    inline void write(Level lv, const char* fmt, ...) {
        const char* tag =
            lv == Level::Info ? "[INFO ]" :
            lv == Level::Warn ? "[WARN ]" :
            "[ERROR]";
        std::printf("%s ", tag);
        va_list ap; va_start(ap, fmt);
        std::vprintf(fmt, ap);
        va_end(ap);
        std::printf("\n");
        std::fflush(stdout);
    }

} // namespace grit::log

#define GRIT_INFO(...)  ::grit::log::write(::grit::log::Level::Info,  __VA_ARGS__)
#define GRIT_WARN(...)  ::grit::log::write(::grit::log::Level::Warn,  __VA_ARGS__)
#define GRIT_ERROR(...) ::grit::log::write(::grit::log::Level::Error, __VA_ARGS__)