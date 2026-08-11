#pragma once

#include <functional>
#include <string_view>

namespace editor_core {

struct EditorContext; // forward declaration — log() only needs a reference.

/// @brief Severity of a non-fatal event reported through LogSink.
///
/// Logging is purely informational/diagnostic: it never replaces the
/// std::expected<T, EditorError> error path for recoverable failures.
/// LogLevel::Warning is used for tolerated-but-notable situations (e.g. a
/// non-planar n-gon accepted silently by the triangulator).
enum class LogLevel { Info, Warning, Error };

/// @brief User-supplied sink for diagnostic events.
///
/// A free function/callable, not a global — EditorContext carries its own
/// LogSink instance, explicitly passed to every Core function that needs
/// it. An empty (default-constructed) LogSink means logging is disabled.
using LogSink = std::function<void(LogLevel level, std::string_view context,
                                   std::string_view message)>;

/// @brief Reports a diagnostic event through the context's LogSink, if any.
///
/// No-op when @c ctx.sink is empty (unset). Never throws — a misbehaving
/// sink is the caller's responsibility, not the Core's.
///
/// @param ctx     Editor context carrying the LogSink.
/// @param level   Severity of the event.
/// @param context Identifies the operation/component emitting the event
///                (e.g. "obj_reader::load_obj").
/// @param message Human-readable description of the event.
void log(const EditorContext &ctx, LogLevel level, std::string_view context,
         std::string_view message);

} // namespace editor_core
