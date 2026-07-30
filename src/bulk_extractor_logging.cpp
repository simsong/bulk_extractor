#include "config.h"

#define SPDLOG_HEADER_ONLY
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include "bulk_extractor_logging.h"

#include <algorithm>
#include <cctype>
#include <memory>
#include <mutex>
#include <stdexcept>

namespace {

std::shared_ptr<spdlog::logger> logger;
std::mutex logger_mutex;

spdlog::level::level_enum to_spdlog_level(bulk_extractor::logging::level value)
{
    using level = bulk_extractor::logging::level;
    switch (value) {
    case level::trace: return spdlog::level::trace;
    case level::debug: return spdlog::level::debug;
    case level::info: return spdlog::level::info;
    case level::warning: return spdlog::level::warn;
    case level::error: return spdlog::level::err;
    case level::critical: return spdlog::level::critical;
    case level::off: return spdlog::level::off;
    }
    throw std::logic_error("unknown log level");
}

bulk_extractor::logging::level parse_level(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    using level = bulk_extractor::logging::level;
    if (value == "trace") return level::trace;
    if (value == "debug") return level::debug;
    if (value == "info") return level::info;
    if (value == "warning" || value == "warn") return level::warning;
    if (value == "error") return level::error;
    if (value == "critical") return level::critical;
    if (value == "off") return level::off;
    throw std::invalid_argument("invalid log level: " + value);
}

}

namespace bulk_extractor::logging {

level resolve_level(const std::optional<std::string> &command_line_level,
                    const char *environment_level, bool debug_requested)
{
    if (command_line_level) return parse_level(*command_line_level);
    if (environment_level && *environment_level) return parse_level(environment_level);
    return debug_requested ? level::debug : level::info;
}

std::string_view level_name(level value)
{
    switch (value) {
    case level::trace: return "trace";
    case level::debug: return "debug";
    case level::info: return "info";
    case level::warning: return "warning";
    case level::error: return "error";
    case level::critical: return "critical";
    case level::off: return "off";
    }
    throw std::logic_error("unknown log level");
}

void initialize(const std::filesystem::path &output_directory,
                const std::optional<std::filesystem::path> &requested_path,
                level value)
{
    const auto path = requested_path.value_or(output_directory / "bulk_extractor.log");
    std::lock_guard<std::mutex> lock(logger_mutex);
    logger.reset();
    spdlog::drop("bulk_extractor");
    try {
        logger = spdlog::basic_logger_mt("bulk_extractor", path.string(), true);
        logger->set_level(to_spdlog_level(value));
        logger->flush_on(spdlog::level::warn);
        logger->set_pattern("%Y-%m-%dT%H:%M:%S.%e%z [%l] [thread %t] %v");
    }
    catch (const spdlog::spdlog_ex &error) {
        throw std::runtime_error("cannot open diagnostic log '" + path.string() + "': " + error.what());
    }
}

void shutdown()
{
    std::lock_guard<std::mutex> lock(logger_mutex);
    if (logger) logger->flush();
    logger.reset();
    spdlog::drop("bulk_extractor");
}

void write(level value, std::string_view component, std::string_view message)
{
    std::lock_guard<std::mutex> lock(logger_mutex);
    if (!logger) return;
    logger->log(to_spdlog_level(value), "[{}] {}", component, message);
}

}
