# PC build of the Rubik's Solver: table generator, tests and console.
# (The Pico build uses CMakeLists.txt instead.)
#
#   make tables   regenerate src/thistle_tables.h
#   make test     build and run the tests
#   make          build everything (output in build_pc/)
#   make run      start the console on the PC

CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra -Isrc
OUT     := build_pc
TABLES  := src/thistle_tables.h
CORE    := src/cube.c src/coords.c
HEADERS := src/rubiks_constants.h src/cube.h src/coords.h src/solver.h

all: $(OUT)/rubiks $(OUT)/test_solver

tables: $(OUT)/gen_tables
	$(OUT)/gen_tables $(TABLES)

test: $(OUT)/test_solver
	$(OUT)/test_solver

run: $(OUT)/rubiks
	$(OUT)/rubiks

$(OUT)/gen_tables: tools/gen_tables.c $(CORE) $(HEADERS)
	@mkdir -p $(OUT)
	$(CC) $(CFLAGS) -o $@ tools/gen_tables.c $(CORE)

$(TABLES):
	$(MAKE) tables

$(OUT)/rubiks: src/main.c src/solver.c src/display.c src/motion.c $(CORE) $(HEADERS) $(TABLES)
	@mkdir -p $(OUT)
	$(CC) $(CFLAGS) -o $@ src/main.c src/solver.c src/display.c src/motion.c $(CORE)

$(OUT)/test_solver: tests/test_solver.c src/solver.c $(CORE) $(HEADERS) $(TABLES)
	@mkdir -p $(OUT)
	$(CC) $(CFLAGS) -o $@ tests/test_solver.c src/solver.c $(CORE)

clean:
	rm -rf $(OUT)

.PHONY: all tables test run clean
