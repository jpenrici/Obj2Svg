# client/CMakeLists.txt

# EditorClient — window, orbital camera, input handling and SVG export command,
# built on top of Raylib and the EditorAPI boundary.

find_package(raylib 5.0 QUIET)
if(NOT raylib_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5)
  set(BUILD_EXAMPLES
      OFF
      CACHE BOOL "" FORCE)
  set(BUILD_GAMES
      OFF
      CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(raylib)
endif()

add_executable(
  EditorClient src/main.cpp src/mesh_view.cpp src/svg_export_command.cpp
               src/camera_controller.cpp)

target_link_libraries(EditorClient PRIVATE Obj2Svg::editor_api raylib
                                           obj2svg_warnings)

target_compile_features(EditorClient PUBLIC cxx_std_26)
