# Rubik's Solver – solve logic

Thistlethwaite solver that runs entirely on the Pico, with its lookup tables
in flash. No PC is needed at run time: the sticker map is entered with an IR
remote and every screen is shown on a TFT. The IR, display and movement
layers are placeholders only; the PC is used just to generate the tables and
run the tests.

## Files

| File | Purpose |
| --- | --- |
| `src/rubiks_constants.h` | Every shared constant, with descriptions |
| `src/cube.c/.h` | Cube state, face-turn tables, sticker strings, validation |
| `src/coords.c/.h` | Table index ("coordinate") each phase measures |
| `src/solver.c/.h` | Four-phase solver that follows decreasing table distances |
| `src/thistle_tables.h` | **Generated** lookup tables (about 970 KB of flash) |
| `src/app.c/.h` | Sticker entry, solving and solution stepping (no hardware, tested on PC) |
| `src/main.c` | Pico main loop: poll input, update the app, redraw the display |
| `src/input.c/.h` | IR remote placeholder; defines the input events |
| `src/display.c/.h` | TFT placeholder; one function per screen |
| `src/motion.c/.h` | Movement placeholder |
| `pc/CMakeLists.txt` | PC build (tables, tests) for Mac/Windows/Linux |
| `tools/gen_tables.c` | PC program that builds the tables by breadth-first search |
| `tests/test_solver.c` | PC tests, including thousands of random cubes |

## PC (macOS, Windows, Linux)

Only needed to regenerate the tables or run the tests. Needs CMake, Ninja and
a C compiler (Xcode tools on Mac; MSVC or MinGW on Windows).

    cmake -S pc -B build_pc -G Ninja
    cmake --build build_pc                          # build everything
    cmake --build build_pc --target tables          # regenerate src/thistle_tables.h
    ctest --test-dir build_pc --output-on-failure   # run the tests (includes the app)

If you have `make`, `make`, `make tables` and `make test` do the same.
On Windows with MSVC, run these from a "Developer PowerShell for VS" so the compiler is found.

## Pico

Open this folder with the Raspberry Pi Pico VS Code extension and build, or:

    cmake -S . -B build -G Ninja
    cmake --build build

Flash `build/RubiksSolver.uf2`. The Pico starts on its own; no serial
connection is needed. To watch state changes while the IR and TFT code is
being written, configure with `-DRUBIKS_DEBUG_USB=ON` and open the USB serial
port (output only).

## Using it

| Screen | Buttons (input events) |
| --- | --- |
| Editing | a colour paints the selected sticker and moves on; NEXT / PREV move one sticker; NEXT_FACE / PREV_FACE jump a face; BACK erases the previous sticker; OK solves; CLEAR starts over |
| Error | any button returns to editing (stickers kept) |
| Solution | NEXT / PREV step through the moves; OK or BACK returns to editing |

Stickers are entered in the order shown in `src/cube.h` (faces U R F D L B,
each read left to right, top to bottom). A blank cube has only the centres
filled in (U white, R red, F green, D yellow, L orange, B blue); repaint them
if the cube is held differently. The IR driver decides which remote button
produces each event (`src/input.h`).

## Phases

| Phase | Allowed moves | Goal | Table entries | Longest |
| --- | --- | --- | --- | --- |
| 1 | all | every edge flipped correctly | 2,048 | 7 |
| 2 | U D L R F2 B2 | corners untwisted, E-slice edges in the E slice | 1,082,565 | 10 |
| 3 | U D L2 R2 F2 B2 | every piece reachable with half turns | 29,400 | 13 |
| 4 | U2 D2 L2 R2 F2 B2 | solved | 663,552 | 15 |

Each table entry is a 4-bit distance to the phase goal. Worst case is 45
moves; random cubes average about 31.
