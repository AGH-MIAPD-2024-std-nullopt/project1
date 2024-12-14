#pragma once

#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#include <fmt/core.h>

namespace logger {
    namespace {
        inline std::string datetimeStr() {
            const auto now = std::chrono::system_clock::now();
            const auto nowTime = std::chrono::system_clock::to_time_t(now);
            const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
            std::tm nowTm = *std::localtime(&nowTime);
            char buf[20];
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &nowTm);
            return fmt::format("[{}.{:03}]", buf, nowMs.count());
        }

        inline std::string format(const std::string& level, const std::string& msg) {
            return fmt::format("{} WEBSERVER/{}: {}", datetimeStr(), level, msg);
        }
    }   // namespace

    inline void error(const std::string& msg) {
        std::cerr << format("ERROR", msg) << std::endl;
    }

    inline void debug(const std::string& msg) {
        std::cout << format("DEBUG", msg) << std::endl;
    }
} // namespace log