# PC tools for the Rubik's Solver: table generator and tests.
# The program itself only runs on the Pico (see CMakeLists.txt).
#
#   make tables   regenerate src/thistle_tables.h
#   make test     build and run the tests
#   make          build everything (output in build_pc/)

CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra -Isrc
OUT     := build_pc
TABLES  := src/thistle_tables.h
CORE    := src/cube.c src/coords.c
HEADERS := src/rubiks_constants.h src/cube.h src/coords.h src/solver.h src/app.h src/input.h

all: $(OUT)/test_solver

tables: $(OUT)/gen_tables
	$(OUT)/gen_tables $(TABLES)

test: $(OUT)/test_solver
	$(OUT)/test_solver

$(OUT)/gen_tables: tools/gen_tables.c $(CORE) $(HEADERS)
	@mkdir -p $(OUT)
	$(CC) $(CFLAGS) -o $@ tools/gen_tables.c $(CORE)

$(TABLES):
	$(MAKE) tables

$(OUT)/test_solver: tests/test_solver.c src/solver.c src/app.c $(CORE) $(HEADERS) $(TABLES)
	@mkdir -p $(OUT)
	$(CC) $(CFLAGS) -o $@ tests/test_solver.c src/solver.c src/app.c $(CORE)

clean:
	rm -rf $(OUT)

.PHONY: all tables test clean
