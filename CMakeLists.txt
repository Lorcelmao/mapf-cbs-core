cmake_minimum_required(VERSION 3.21)
project(mapf_cbs_core LANGUAGES CXX)

# Tests and benchmarks are built only when this repository is the top-level
# project, so a parent repository that adds it as a subdirectory gets just the
# library and the solver.
option(MAPF_CORE_BUILD_TESTS "Build the unit tests and benchmarks of mapf-cbs-core"
       ${PROJECT_IS_TOP_LEVEL})

# Benchmark numbers must come from optimized builds, so Release is the default
# when this repository is built on its own. A parent project keeps its choice.
if(PROJECT_IS_TOP_LEVEL AND NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(MSVC)
  set(MAPF_WARNING_FLAGS /W4)
else()
  set(MAPF_WARNING_FLAGS -Wall -Wextra)
endif()

# Applies the project's language level and warnings to one of its targets.
# MinGW executables are linked statically: they otherwise need the compiler's
# runtime DLLs on PATH, which breaks ctest and the benchmark runner when they
# start from a plain shell.
function(mapf_configure_target target)
  target_compile_features(${target} PUBLIC cxx_std_17)
  set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
  target_compile_options(${target} PRIVATE ${MAPF_WARNING_FLAGS})
  get_target_property(target_type ${target} TYPE)
  if(MINGW AND target_type STREQUAL "EXECUTABLE")
    target_link_options(${target} PRIVATE -static)
  endif()
endfunction()

# Everything except main.cpp lives in this library so tests can link it.
add_library(mapf_core STATIC
  src/app/cli_options.cpp
  src/app/peak_memory.cpp
  src/app/result_writer.cpp
  src/core/grid.cpp
  src/core/instance.cpp
  src/lowlevel/constraint_table.cpp
  src/lowlevel/heuristic_table.cpp
  src/lowlevel/mdd.cpp
  src/lowlevel/space_time_astar.cpp
)
target_include_directories(mapf_core PUBLIC src)
mapf_configure_target(mapf_core)

add_executable(mapf_solver src/app/main.cpp)
target_link_libraries(mapf_solver PRIVATE mapf_core)
mapf_configure_target(mapf_solver)

if(MAPF_CORE_BUILD_TESTS)
  # Low-level micro-benchmark. `cmake --build build --target bench_lowlevel`
  # runs it on the benchmark map that ships with the tests.
  add_executable(mapf_bench_lowlevel bench/bench_find_path.cpp)
  target_link_libraries(mapf_bench_lowlevel PRIVATE mapf_core)
  mapf_configure_target(mapf_bench_lowlevel)
  add_custom_target(bench_lowlevel
    COMMAND mapf_bench_lowlevel
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/movingai/random-32-32-20.map
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/movingai/random-32-32-20-random-1.scen
            409
    USES_TERMINAL
  )

  enable_testing()
  add_executable(mapf_tests
    tests/test_main.cpp
    tests/test_cli_options.cpp
    tests/test_constraint_table.cpp
    tests/test_grid.cpp
    tests/test_heuristic_table.cpp
    tests/test_instance.cpp
    tests/test_mdd.cpp
    tests/test_peak_memory.cpp
    tests/test_result_writer.cpp
    tests/test_space_time_astar.cpp
  )
  target_link_libraries(mapf_tests PRIVATE mapf_core)
  target_include_directories(mapf_tests SYSTEM PRIVATE third_party/doctest)
  target_compile_definitions(mapf_tests PRIVATE
    MAPF_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/data"
  )
  mapf_configure_target(mapf_tests)
  add_test(NAME mapf_tests COMMAND mapf_tests)

  # End-to-end check of the executable's exit codes and JSON output.
  find_package(Python3 COMPONENTS Interpreter)
  if(Python3_Interpreter_FOUND)
    add_test(NAME mapf_solver_cli
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/check-solver-cli.py
              $<TARGET_FILE:mapf_solver> ${CMAKE_CURRENT_SOURCE_DIR}/tests/data
    )
  endif()
endif()
