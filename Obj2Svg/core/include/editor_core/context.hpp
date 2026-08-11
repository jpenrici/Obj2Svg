#pragma once

#include "editor_core/logger.hpp"

namespace editor_core {

/// @brief Explicit, stateless-by-design context passed to every Core
///        function that needs shared facilities.
///
/// EditorContext replaces global/singleton state: callers construct one
/// (or reuse one) and pass it by const reference (e.g.
/// `load_obj(context, path)`). Fields beyond @c sink are reserved for
/// future cross-cutting concerns (allocator, locale, cache, statistics)
/// and are not used by the Core yet.
struct EditorContext {
  LogSink
      sink; ///< Diagnostic sink; default-constructed (empty) disables logging.

  // Reserved for future use — intentionally absent for now:
  //   Allocator   allocator;
  //   Locale      locale;
  //   Cache       cache;
};

} // namespace editor_core
