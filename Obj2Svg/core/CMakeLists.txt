# core/CMakeLists.txt

# EditorCore — geometry, transforms, projection, triangulation and OBJ/SVG I/O.
# No dependency on the API boundary or the client; pure C++26.

add_library(
  editor_core STATIC
  # -- Headers--
  include/editor_core/context.hpp
  include/editor_core/error.hpp
  include/editor_core/logger.hpp
  include/editor_core/geometry/mesh.hpp
  include/editor_core/geometry/projection.hpp
  include/editor_core/geometry/transform.hpp
  include/editor_core/geometry/triangulator.hpp
  include/editor_core/io/obj_reader.hpp
  include/editor_core/io/svg_writer.hpp
  src/geometry/geometry_internal.hpp
  # -- Sources --
  src/context.cpp
  src/logger.cpp
  src/geometry/mesh.cpp
  src/geometry/geometry_internal.cpp
  src/geometry/projection.cpp
  src/geometry/transform.cpp
  src/geometry/triangulator.cpp
  src/io/obj_reader.cpp
  src/io/svg_writer.cpp)

add_library(Obj2Svg::editor_core ALIAS editor_core)

target_include_directories(editor_core
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(editor_core PRIVATE obj2svg_warnings)

target_compile_features(editor_core PUBLIC cxx_std_26)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(OBJ2SVG_BUILD_TESTS)
  enable_testing()

  set(OBJ2SVG_CORE_TEST_SOURCES
      tests/geometry/test_projection.cpp tests/geometry/test_transform.cpp
      tests/geometry/test_triangulator.cpp tests/io/test_obj_reader.cpp
      tests/io/test_svg_writer.cpp)

  foreach(test_src ${OBJ2SVG_CORE_TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE Obj2Svg::editor_core
                                               obj2svg_warnings)
    if(test_name STREQUAL "test_obj_reader")
      target_compile_definitions(
        ${test_name}
        PRIVATE FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/io/fixtures")
    endif()

    add_test(NAME ${test_name} COMMAND ${test_name})
  endforeach()
endif()
