#pragma once
#include "LogSink.h"
#include <cstdio>
#include <cstdarg>
#include <string>

namespace grit::log {

    enum class Level { Info, Warn, Error };

    inline void write(Level lv, const char* fmt, ...) {
        const char* tag =
            lv == Level::Info ? "[INFO ]" :
            lv == Level::Warn ? "[WARN ]" :
            "[ERROR]";

        // Один va_list, две КОПИИ через va_copy
        va_list ap;
        va_start(ap, fmt);

        // 1) Копия для stdout
        va_list ap_stdout;
        va_copy(ap_stdout, ap);

        // 2) Копия для буфера ImGui-консоли
        va_list ap_buf;
        va_copy(ap_buf, ap);

        // Печать в stdout
        std::printf("%s ", tag);
        std::vprintf(fmt, ap_stdout);
        std::printf("\n");
        std::fflush(stdout);
        va_end(ap_stdout);

        // Формируем строку для LogSink
        char buf[1024];
        int n = std::vsnprintf(buf, sizeof(buf), fmt, ap_buf);
        va_end(ap_buf);

        va_end(ap);

        if (n < 0) n = 0;
        if (n >= (int)sizeof(buf)) n = (int)sizeof(buf) - 1;

        std::string text(buf, (size_t)n);

        LogLevel sl =
            lv == Level::Info ? LogLevel::Info :
            lv == Level::Warn ? LogLevel::Warn :
            LogLevel::Error;

        LogSink::get().push(sl, std::move(text));
    }

} // namespace grit::log

#define GRIT_INFO(...)  ::grit::log::write(::grit::log::Level::Info,  __VA_ARGS__)
#define GRIT_WARN(...)  ::grit::log::write(::grit::log::Level::Warn,  __VA_ARGS__)
#define GRIT_ERROR(...) ::grit::log::write(::grit::log::Level::Error, __VA_ARGS__)