#include "editor_core/logger.hpp"
#include "editor_core/context.hpp"

namespace editor_core {

void log(const EditorContext &ctx, LogLevel level, std::string_view context,
         std::string_view message) {
  if (ctx.sink) {
    ctx.sink(level, context, message);
  }
}

} // namespace editor_core
