# Obj2Svg

OBJ file visualization and SVG export tool using C++ and Raylib.

See more: https://en.wikipedia.org/wiki/Wavefront_.obj_file

## Detail

Obj2Svg handles exclusively the **OBJ** file format.

## Architecture

```
[ UI / Visualization Layer ] --(C-API)--> [ Core Library ] --> [ File Reader/Writer ]
   (Raylib)                             (Mesh & Operations)          (OBJ / SVG)
```

- **EditorCore** — geometry (mesh, transform, projection, triangulation),
  I/O (OBJ/SVG), error handling and context. No exceptions, no global
  state; recoverable errors use `std::expected<T, EditorError>`.
- **EditorAPI** — `extern "C"` boundary with opaque pointers, granular
  enough to expose the full mesh (vertices, normals, faces, edges).
- **EditorClient** — window, orbital camera, input handling, mesh upload
  to Raylib, and SVG export command.

## Requirements

- C++26 (GCC 16+)
- CMake 3.28+
- Raylib 5.x+

## Building

```bash
cmake -B build -S .
cmake --build build
```

## Project Structure

```
Obj2Svg/
├── core/      # Engine library (geometry + I/O)
├── api/       # Extern "C" boundary
├── client/    # Raylib visualization/interaction layer
└── resources/ # OBJ files for samples
```

## Display

![display](https://github.com/jpenrici/Obj2Svg/blob/main/display/display.png)
