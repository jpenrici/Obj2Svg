// svg_export_command.cpp
//
// TODO (Phase 5): not implemented yet.
//
// This command needs to build an editor_core::CameraView from the current
// OrbitalCamera state (view/projection matrices + viewport size), call
// editor_core::project_mesh and editor_core::io::write_svg_file on the
// mesh currently held by MeshView's EditorHandle, and write the result to
// a user-chosen path.
//
// editor_api.h (the C boundary) does not expose projection or SVG export
// yet — Phase 2 deliberately scoped that out, since it depends on camera
// state that only exists once the client has one (see
// docs/HISTORICO.md, Fase 2 section, "Fora de escopo desta fase").
// Implementing this command means extending editor_api.h first, e.g. with
// something like:
//
//   bool editor_export_svg(EditorHandle handle,
//                           const float view_matrix[16],
//                           const float projection_matrix[16],
//                           int viewport_width, int viewport_height,
//                           const char* filepath,
//                           /* SvgExportOptions mirrored as C-compatible
//                              fields or a dedicated struct */);
//
// which internally builds a CameraView, calls project_mesh on
// handle->mesh, and calls write_svg_file — mirroring how editor_triangulate
// wraps editor_core::triangulate today.
