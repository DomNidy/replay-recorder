#pragma once

#include <spdlog/spdlog.h>
#include <string>

// Extract filename without path
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

// Macros for logging with automatic file and line information
#define LOG_TRACE(...) spdlog::trace("\033[90m[{}:{}]\033[0m {}", __FILENAME__, __LINE__, fmt::format(__VA_ARGS__))
#define LOG_DEBUG(...) spdlog::debug("\033[90m[{}:{}]\033[0m {}", __FILENAME__, __LINE__, fmt::format(__VA_ARGS__))
#define LOG_INFO(...) spdlog::info("\033[90m[{}:{}]\033[0m {}", __FILENAME__, __LINE__, fmt::format(__VA_ARGS__))
#define LOG_WARN(...) spdlog::warn("\033[90m[{}:{}]\033[0m {}", __FILENAME__, __LINE__, fmt::format(__VA_ARGS__))
#define LOG_ERROR(...) spdlog::error("\033[90m[{}:{}]\033[0m {}", __FILENAME__, __LINE__, fmt::format(__VA_ARGS__))
#define LOG_CRITICAL(...)                                                                                              \
    spdlog::critical("\033[90m[{}:{}]\033[0m {}", __FILENAME__, __LINE__, fmt::format(__VA_ARGS__))

// Class-based logging macros
#define LOG_CLASS_TRACE(className, ...)                                                                                \
    spdlog::trace("\033[90m[{}:{}]\033[0m [\033[1m{}\033[0m] {}", __FILENAME__, __LINE__, className,                   \
                  fmt::format(__VA_ARGS__))
#define LOG_CLASS_DEBUG(className, ...)                                                                                \
    spdlog::debug("\033[90m[{}:{}]\033[0m [\033[1m{}\033[0m] {}", __FILENAME__, __LINE__, className,                   \
                  fmt::format(__VA_ARGS__))
#define LOG_CLASS_INFO(className, ...)                                                                                 \
    spdlog::info("\033[90m[{}:{}]\033[0m [\033[1m{}\033[0m] {}", __FILENAME__, __LINE__, className,                    \
                 fmt::format(__VA_ARGS__))
#define LOG_CLASS_WARN(className, ...)                                                                                 \
    spdlog::warn("\033[90m[{}:{}]\033[0m [\033[1m{}\033[0m] {}", __FILENAME__, __LINE__, className,                    \
                 fmt::format(__VA_ARGS__))
#define LOG_CLASS_ERROR(className, ...)                                                                                \
    spdlog::error("\033[90m[{}:{}]\033[0m [\033[1m{}\033[0m] {}", __FILENAME__, __LINE__, className,                   \
                  fmt::format(__VA_ARGS__))
#define LOG_CLASS_CRITICAL(className, ...)                                                                             \
    spdlog::critical("\033[90m[{}:{}]\033[0m [\033[1m{}\033[0m] {}", __FILENAME__, __LINE__, className,                \
                     fmt::format(__VA_ARGS__))

namespace RP
{
namespace Logging
{

// Initialize logging with appropriate settings
inline void initLogging(spdlog::level::level_enum level = spdlog::level::info)
{
    spdlog::set_level(level);
    spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
}

} // namespace Logging
} // namespace RP