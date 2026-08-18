#pragma once

#include <cstddef>
#include <optional>
#include <string>

namespace editor_core {

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

struct EditorError {
  ErrorCode code;
  std::string context;
  std::string message;
  std::optional<std::size_t> line;
};

} // namespace editor_core
