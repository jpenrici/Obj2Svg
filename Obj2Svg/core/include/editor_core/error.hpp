#pragma once

#include <cstddef>
#include <optional>
#include <string>

namespace editor_core {

/// @brief Recoverable failure categories produced by EditorCore.
///
/// Every recoverable failure in the Core is represented by one of these
/// codes, wrapped in an EditorError and returned via
/// std::expected<T, EditorError>. The Core never throws exceptions for
/// recoverable failures.
enum class ErrorCode {
  FileNotFound,  ///< Requested file path does not exist or cannot be opened.
  InvalidOBJ,    ///< Malformed OBJ syntax that doesn't fit a more specific code
                 ///< below.
  InvalidFace,   ///< Face violates a structural invariant (e.g. mismatched
                 ///< vertex/normal index counts).
  InvalidVertex, ///< Malformed vertex ("v") line.
  InvalidIndex,  ///< Zero index, out-of-range index, or unresolvable relative
                 ///< (negative) index.
  EmptyMesh, ///< File parsed successfully but produced no vertices or no faces.
  DegenerateFace, ///< Face with fewer than 3 unique vertices, or ~0 area after
                  ///< triangulation.
  IOError, ///< Low-level I/O failure (read/write/permission) not covered by
           ///< FileNotFound.
  InvalidProjection, ///< Camera/projection parameters that cannot produce a
                     ///< valid ProjectedMesh.
  InternalError ///< Unexpected internal failure — should not happen in correct
                ///< usage.
};

/// @brief Structured, recoverable error returned by fallible Core operations.
///
/// @c context identifies the operation/component that produced the error
/// (e.g. "obj_reader::load_obj"), @c message is a human-readable
/// description, and @c line is populated when the error originates from
/// parsing a specific line of a text file (e.g. an OBJ file).
struct EditorError {
  ErrorCode code;
  std::string context;
  std::string message;
  std::optional<std::size_t> line;
};

} // namespace editor_core
