# Rubik's Solver - Pico build. Runs standalone: IR remote in, TFT out,
# lookup tables in flash. No USB serial connection is needed.

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
    src/app.c           # sticker entry, solving, solution stepping (no hardware)
    src/cube.c
    src/coords.c
    src/solver.c        # includes the generated lookup tables (src/thistle_tables.h)
    src/input.c         # placeholder: IR remote
    src/display.c       # placeholder: TFT
    src/motion.c        # placeholder
)

target_include_directories(RubiksSolver PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)

pico_set_program_name(RubiksSolver "RubiksSolver")
pico_set_program_version(RubiksSolver "1.0")

# The Pico runs on its own, so stdio is off. Turn this on to print state
# changes over USB serial while developing (output only, never required).
option(RUBIKS_DEBUG_USB "Print debug output over USB serial" OFF)
if (RUBIKS_DEBUG_USB)
    target_compile_definitions(RubiksSolver PRIVATE RUBIKS_DEBUG_LOG=1)
    pico_enable_stdio_usb(RubiksSolver 1)
else()
    pico_enable_stdio_usb(RubiksSolver 0)
endif()
pico_enable_stdio_uart(RubiksSolver 0)

target_link_libraries(RubiksSolver pico_stdlib)

pico_add_extra_outputs(RubiksSolver)
