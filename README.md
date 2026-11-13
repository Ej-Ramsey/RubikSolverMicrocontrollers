# Rubik's Solver – solve logic

Thistlethwaite solver driven by lookup tables stored in the Pico's flash.
There is no physical build: the cube state is typed over USB serial and the
answer is printed as a move list. The display and movement layers are
placeholders only.

## Files

| File | Purpose |
| --- | --- |
| `src/rubiks_constants.h` | Every shared constant, with descriptions |
| `src/cube.c/.h` | Cube state, face-turn tables, sticker strings, validation |
| `src/coords.c/.h` | Table index ("coordinate") each phase measures |
| `src/solver.c/.h` | Four-phase solver that follows decreasing table distances |
| `src/thistle_tables.h` | **Generated** lookup tables (about 970 KB of flash) |
| `src/main.c` | Serial console (Pico USB serial or PC terminal) |
| `src/display.c/.h`, `src/motion.c/.h` | Placeholders, intentionally empty |
| `pc/CMakeLists.txt` | PC build (tables, tests, console) for Mac/Windows/Linux |
| `tools/gen_tables.c` | PC program that builds the tables by breadth-first search |
| `tests/test_solver.c` | PC tests, including thousands of random cubes |

## PC (macOS, Windows, Linux)

Needs CMake, Ninja and a C compiler (Xcode tools on Mac; MSVC or MinGW on Windows).

    cmake -S pc -B build_pc -G Ninja
    cmake --build build_pc                          # build everything
    cmake --build build_pc --target tables          # regenerate src/thistle_tables.h
    ctest --test-dir build_pc --output-on-failure   # run the tests
    build_pc/rubiks                                 # start the console (build_pc\\rubiks.exe on Windows)

If you have `make`, `make`, `make tables`, `make test` and `make run` do the same.
On Windows with MSVC, run these from a "Developer PowerShell for VS" so the compiler is found.

## Pico

Open this folder with the Raspberry Pi Pico VS Code extension and build, or:

    cmake -S . -B build -G Ninja
    cmake --build build

Flash `build/RubiksSolver.uf2`, open the USB serial port, and type either a
scramble (`R U2 F' L D2 B`) or 54 stickers (order described in `src/cube.h`).

## Phases

| Phase | Allowed moves | Goal | Table entries | Longest |
| --- | --- | --- | --- | --- |
| 1 | all | every edge flipped correctly | 2,048 | 7 |
| 2 | U D L R F2 B2 | corners untwisted, E-slice edges in the E slice | 1,082,565 | 10 |
| 3 | U D L2 R2 F2 B2 | every piece reachable with half turns | 29,400 | 13 |
| 4 | U2 D2 L2 R2 F2 B2 | solved | 663,552 | 15 |

Each table entry is a 4-bit distance to the phase goal. Worst case is 45
moves; random cubes average about 31.
