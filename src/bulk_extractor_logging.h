#ifndef BULK_EXTRACTOR_LOGGING_H
#define BULK_EXTRACTOR_LOGGING_H

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace bulk_extractor::logging {

enum class level { trace, debug, info, warning, error, critical, off };

level resolve_level(const std::optional<std::string> &command_line_level,
                    const char *environment_level, bool debug_requested);
std::string_view level_name(level value);

void initialize(const std::filesystem::path &output_directory,
                const std::optional<std::filesystem::path> &requested_path,
                level value);
void shutdown();
void write(level value, std::string_view component, std::string_view message);

}

#endif
