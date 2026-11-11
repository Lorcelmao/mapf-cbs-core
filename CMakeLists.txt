cmake_minimum_required(VERSION 3.20)
project(mapf_cbs_ecbs LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Benchmark numbers must come from optimized builds, so Release is the default.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra)
endif()

# MinGW binaries otherwise need the compiler's runtime DLLs on PATH, which
# breaks ctest and the benchmark runner when they start from a plain shell.
if(MINGW)
  add_link_options(-static)
endif()

# Everything except main.cpp lives in this library so tests can link it.
add_library(mapf_core STATIC
  src/core/grid.cpp
  src/core/instance.cpp
  src/lowlevel/constraint_table.cpp
  src/lowlevel/heuristic_table.cpp
  src/lowlevel/space_time_astar.cpp
)
target_include_directories(mapf_core PUBLIC src)

include(CTest)
if(BUILD_TESTING)
  add_executable(mapf_tests
    tests/test_main.cpp
    tests/test_grid.cpp
    tests/test_instance.cpp
    tests/test_constraint_table.cpp
    tests/test_heuristic_table.cpp
    tests/test_space_time_astar.cpp
  )
  target_link_libraries(mapf_tests PRIVATE mapf_core)
  target_include_directories(mapf_tests SYSTEM PRIVATE third_party/doctest)
  target_compile_definitions(mapf_tests PRIVATE
    MAPF_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/data"
  )
  add_test(NAME mapf_tests COMMAND mapf_tests)
endif()
