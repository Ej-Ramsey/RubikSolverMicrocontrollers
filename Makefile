# Shortcuts for the PC build. The real build is pc/CMakeLists.txt, which works
# on macOS, Windows and Linux; these targets just call CMake for you.
# (The Pico build uses the top-level CMakeLists.txt instead.)
#
#   make          build everything (output in build_pc/)
#   make tables   regenerate src/thistle_tables.h
#   make test     build and run the tests

OUT := build_pc

all: $(OUT)/build.ninja
	cmake --build $(OUT)

$(OUT)/build.ninja:
	cmake -S pc -B $(OUT) -G Ninja

tables: $(OUT)/build.ninja
	cmake --build $(OUT) --target tables

test: all
	ctest --test-dir $(OUT) --output-on-failure

clean:
	cmake -E rm -rf $(OUT)

.PHONY: all tables test clean
