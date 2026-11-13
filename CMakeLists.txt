# Rubik's Solver - Pico build (solve logic only, driven over USB serial)

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(RubiksSolver C CXX ASM)

pico_sdk_init()

add_executable(RubiksSolver
    src/main.c
    src/cube.c
    src/coords.c
    src/solver.c        # includes the generated lookup tables (src/thistle_tables.h)
    src/display.c       # placeholder
    src/motion.c        # placeholder
)

# Tells main.c to use the Pico's USB serial and timer instead of the PC's.
target_compile_definitions(RubiksSolver PRIVATE PICO_BUILD=1)

target_include_directories(RubiksSolver PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)

pico_set_program_name(RubiksSolver "RubiksSolver")
pico_set_program_version(RubiksSolver "1.0")

# The console runs over USB serial, so the Pico only needs its USB cable.
pico_enable_stdio_uart(RubiksSolver 0)
pico_enable_stdio_usb(RubiksSolver 1)

target_link_libraries(RubiksSolver pico_stdlib)

pico_add_extra_outputs(RubiksSolver)
