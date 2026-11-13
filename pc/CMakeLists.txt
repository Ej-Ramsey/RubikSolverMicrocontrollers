# PC build of the Rubik's Solver: table generator and tests.
# Works on macOS, Windows and Linux. The program itself only runs on the
# Pico (see ../CMakeLists.txt).
#
#   cmake -S pc -B build_pc -G Ninja
#   cmake --build build_pc                      build everything
#   cmake --build build_pc --target tables      regenerate src/thistle_tables.h
#   ctest --test-dir build_pc --output-on-failure   run the tests

cmake_minimum_required(VERSION 3.13)
project(RubiksSolverPC C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Put the programs straight in the build folder (build_pc/test_solver, not build_pc/Debug/...)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
foreach(cfg Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${cfg} CFG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG} ${CMAKE_BINARY_DIR})
endforeach()

set(ROOT ${CMAKE_CURRENT_LIST_DIR}/..)
set(SRC  ${ROOT}/src)

if(MSVC)
    add_compile_options(/W4)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra)
endif()

# Shared cube logic
add_library(cube_core STATIC ${SRC}/cube.c ${SRC}/coords.c)
target_include_directories(cube_core PUBLIC ${SRC})

# Table generator: builds src/thistle_tables.h by breadth-first search
add_executable(gen_tables ${ROOT}/tools/gen_tables.c)
target_link_libraries(gen_tables cube_core)

add_custom_target(tables
    COMMAND gen_tables ${SRC}/thistle_tables.h
    DEPENDS gen_tables
    COMMENT "Regenerating src/thistle_tables.h"
    VERBATIM)

# Solver (uses the generated tables)
add_library(solver STATIC ${SRC}/solver.c)
target_link_libraries(solver PUBLIC cube_core)

# Tests (the app is hardware-independent, so it is tested here too)
add_executable(test_solver ${ROOT}/tests/test_solver.c ${SRC}/app.c)
target_link_libraries(test_solver solver)

enable_testing()
add_test(NAME solver COMMAND test_solver)
