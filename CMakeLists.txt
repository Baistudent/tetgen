cmake_minimum_required(VERSION 3.5)
project(tetgen)

option(BUILD_EXECUTABLE "Build tetgen executable" OFF)
option(BUILD_LIBRARY    "Build libtetgen library" ON)
option(BUILD_DENSITY_EXAMPLE "Build custom density region example" OFF)

if(BUILD_LIBRARY)
  add_library(tetgen STATIC tetgen.cxx predicates.cxx)
  target_compile_definitions(tetgen PRIVATE -DTETLIBRARY)
  # Generate position independent code
  set_target_properties(tetgen PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()


if(BUILD_EXECUTABLE)
  add_executable(tetgen_exec tetgen.cxx predicates.cxx)
  set_target_properties(tetgen_exec PROPERTIES OUTPUT_NAME tetgen)
endif()

if(BUILD_DENSITY_EXAMPLE)
  if(NOT BUILD_LIBRARY)
    message(FATAL_ERROR "BUILD_DENSITY_EXAMPLE requires BUILD_LIBRARY=ON")
  endif()
  add_executable(density_regions_example examples/custom_density_regions.cxx)
  target_compile_definitions(density_regions_example PRIVATE -DTETLIBRARY)
  target_include_directories(density_regions_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(density_regions_example PRIVATE tetgen)
endif()
