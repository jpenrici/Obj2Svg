#pragma once

#include <functional>
#include <string_view>

namespace editor_core {

struct EditorContext;

enum class LogLevel { Info, Warning, Error };

using LogSink = std::function<void(LogLevel level, std::string_view context,
                                   std::string_view message)>;

void log(const EditorContext &ctx, LogLevel level, std::string_view context,
         std::string_view message);

} // namespace editor_core
