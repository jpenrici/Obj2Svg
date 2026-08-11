# api/CMakeLists.txt EditorAPI — extern "C" boundary over EditorCore, using
# opaque handles.

add_library(
  editor_api STATIC
  # -- Public C header --
  include/editor_api.h
  # -- Source --
  src/editor_api.cpp)

add_library(Obj2Svg::editor_api ALIAS editor_api)

target_include_directories(editor_api
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(
  editor_api
  PUBLIC Obj2Svg::editor_core
  PRIVATE obj2svg_warnings)

target_compile_features(editor_api PUBLIC cxx_std_26)
