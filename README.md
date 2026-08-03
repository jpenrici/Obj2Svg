# Obj2Svg

A modular 3D editor engine written in modern C++, decoupled from its UI
through a C (`extern "C"`) API boundary, with an initial visualization
layer built on Raylib.

This is a learning-focused project centered on computer graphics, clean
software architecture, polygonal mesh manipulation, and C/C++ system
integration.

## Scope

Obj2Svg deals exclusively with the **OBJ** file format — it already
provides indexed vertices, native normal support, and is the most
universal interchange format across tools like Blender, FreeCAD,
MeshLab, Unity, and Unreal.

## Architecture

```
[ UI / Visualization Layer ] --(C-API)--> [ Core Library (C++26) ] --> [ File Reader/Writer ]
   (Raylib)                                  (Mesh & Operations)          (OBJ / SVG)
```

- **EditorCore** — geometry (mesh, transform, projection, triangulation),
  I/O (OBJ/SVG), error handling and context. No exceptions, no global
  state; recoverable errors use `std::expected<T, EditorError>`.
- **EditorAPI** — `extern "C"` boundary with opaque pointers, granular
  enough to expose the full mesh (vertices, normals, faces, edges).
- **EditorClient** — window, orbital camera, input handling, mesh upload
  to Raylib, and SVG export command.

## Development Phases

1. Data structures and OBJ parser
2. C-API boundary (`extern "C"`)
3. Raylib frontend
4. Basic mesh editing (translate/rotate/scale)
5. SVG export (orbital view — wireframe and solid modes)

## Requirements

- C++26 (GCC 16+)
- CMake 3.28+
- Raylib 5.x+
- Git

## Building

```bash
cmake -B build -S .
cmake --build build
```

## Project Structure

```
Obj2Svg/
├── core/     # Engine library (geometry + I/O)
├── api/      # extern "C" boundary
├── client/   # Raylib visualization/interaction layer
└── resources/# Sample OBJ files and SVG export outputs
```

## Status

Early development!
