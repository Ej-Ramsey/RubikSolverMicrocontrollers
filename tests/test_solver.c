/*
 * test_solver.c  (runs on the PC)
 *
 * Checks the cube model, the validator and the solver. The main test solves
 * many random cubes and confirms that applying each answer really gives a
 * solved cube.
 *
 * Usage: test_solver [number of random cubes]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cube.h"
#include "rubiks_constants.h"
#include "solver.h"

/* ------------------------------------------------------------------ */
/* Constants                                                          */
/* ------------------------------------------------------------------ */
#define DEFAULT_RANDOM_CUBES    10000   /* Random cubes solved when no count is given     */
#define RANDOM_SEED             12345   /* Fixed seed so every run tests the same cubes   */
#define RANDOM_SCRAMBLE_MOVES   40      /* Length of the scramble in the round-trip test  */

/* Sticker string of a solved cube after the single move R. */
#define STICKERS_AFTER_R  "UUFUUFUUFRRRRRRRRRFFDFFDFFDDDBDDBDDBLLLLLLLLLUBBUBBUBB"

static int failures = 0;    /* Number of failed checks */

/* Records and reports a failure if `condition` is false, then carries on
 * so that one run shows every problem. */
#define CHECK(condition, description)                               \
    do {                                                            \
        if (!(condition)) {                                         \
            printf("FAIL: %s (line %d)\n", description, __LINE__);  \
            failures++;                                             \
        }                                                           \
    } while (0)

/* ------------------------------------------------------------------ */
/* Helpers                                                            */
/* ------------------------------------------------------------------ */

/* Puts the values in a random order (Fisher-Yates shuffle): each item,
 * from the last to the second, is swapped with a random earlier item or
 * itself. */
static void shuffle(uint8_t *values, uint8_t length)
{
    for (uint8_t i = (uint8_t)(length - 1); i > 0; i--) {
        uint8_t j = (uint8_t)(rand() % (i + 1));
        uint8_t swap = values[i];
        values[i] = values[j];
        values[j] = swap;
    }
}

/* Builds a uniformly random legal cube directly from the cube laws. */
static void random_cube(Cube *cube)
{
    uint8_t twist_sum = 0;
    uint8_t flip_sum = 0;

    /* Random piece positions. If the corner and edge parities differ,
     * swapping two edges fixes it. */
    cube_reset(cube);
    shuffle(cube->cp, NUM_CORNERS);
    shuffle(cube->ep, NUM_EDGES);
    if (permutation_parity(cube->cp, NUM_CORNERS) != permutation_parity(cube->ep, NUM_EDGES)) {
        uint8_t swap = cube->ep[0];
        cube->ep[0] = cube->ep[1];
        cube->ep[1] = swap;
    }

    /* Random twists and flips; the last corner and last edge are set so
     * the totals obey the cube laws. */
    for (uint8_t i = 0; i < NUM_CORNERS - 1; i++) {
        cube->co[i] = (uint8_t)(rand() % CORNER_ORIENTATIONS);
        twist_sum = (uint8_t)(twist_sum + cube->co[i]);
    }
    cube->co[NUM_CORNERS - 1] =
        (uint8_t)((CORNER_ORIENTATIONS - twist_sum % CORNER_ORIENTATIONS) % CORNER_ORIENTATIONS);
    for (uint8_t i = 0; i < NUM_EDGES - 1; i++) {
        cube->eo[i] = (uint8_t)(rand() % EDGE_ORIENTATIONS);
        flip_sum = (uint8_t)(flip_sum + cube->eo[i]);
    }
    cube->eo[NUM_EDGES - 1] = (uint8_t)(flip_sum % EDGE_ORIENTATIONS);
}

/* ------------------------------------------------------------------ */
/* Tests                                                              */
/* ------------------------------------------------------------------ */

/* Checks the face-turn tables and the move text parser. */
static void test_moves(void)
{
    Cube cube;
    char stickers[NUM_STICKERS + 1];

    for (uint8_t move = 0; move < NUM_MOVES; move++) {
        /* A move followed by its inverse changes nothing. */
        cube_reset(&cube);
        cube_apply_move(&cube, move);
        CHECK(!cube_is_solved(&cube), "move changes the cube");
        cube_apply_move(&cube, move_inverse(move));
        CHECK(cube_is_solved(&cube), "move then inverse is the identity");

        /* Any move repeated four times changes nothing. */
        for (uint8_t n = 0; n < QUARTER_TURNS_PER_CYCLE; n++) {
            cube_apply_move(&cube, move);
        }
        CHECK(cube_is_solved(&cube), "move repeated four times is the identity");
    }

    /* (R U R' U') repeated six times is a well-known identity. */
    uint8_t sexy[4];
    CHECK(moves_parse("R U R' U'", sexy, 4) == 4, "parse R U R' U'");
    cube_reset(&cube);
    for (uint8_t n = 0; n < 6; n++) {
        cube_apply_moves(&cube, sexy, 4);
        CHECK((n == 5) == cube_is_solved(&cube), "R U R' U' has order six");
    }

    /* The move tables agree with the sticker layout of a real cube. */
    cube_reset(&cube);
    cube_apply_move(&cube, move_make(FACE_R, 1));
    cube_to_stickers(&cube, stickers);
    CHECK(strcmp(stickers, STICKERS_AFTER_R) == 0, "stickers after R");

    CHECK(moves_parse("R X", sexy, 4) < 0, "reject unknown face");
    CHECK(moves_parse("R3", sexy, 4) < 0, "reject bad suffix");
}

/* Checks sticker-string conversion and that each kind of impossible cube
 * is rejected with the right error. */
static void test_validation(void)
{
    Cube cube;
    Cube parsed;
    char message[MESSAGE_MAX];
    char stickers[NUM_STICKERS + 1];

    /* Sticker strings survive a round trip, with any colour letters. */
    static const char FACES[] = "URFDLB";
    static const char COLORS[] = "WRGYOB";
    for (int n = 0; n < RANDOM_SCRAMBLE_MOVES; n++) {
        random_cube(&cube);
        cube_to_stickers(&cube, stickers);
        for (int i = 0; i < NUM_STICKERS; i++) {
            stickers[i] = COLORS[strchr(FACES, stickers[i]) - FACES];
        }
        CHECK(cube_from_stickers(stickers, &parsed, message, sizeof message) == CUBE_OK,
              "random cube stickers are valid");
        CHECK(cube_equal(&cube, &parsed), "sticker round trip");
    }

    /* Each block below breaks one cube law on an otherwise solved cube. */
    cube_reset(&cube);
    cube.co[0] = 1;
    CHECK(cube_validate(&cube, message, sizeof message) == CUBE_ERR_TWIST, "twisted corner");

    cube_reset(&cube);
    cube.eo[3] = 1;
    CHECK(cube_validate(&cube, message, sizeof message) == CUBE_ERR_FLIP, "flipped edge");

    cube_reset(&cube);
    cube.ep[0] = 1;
    cube.ep[1] = 0;
    CHECK(cube_validate(&cube, message, sizeof message) == CUBE_ERR_PARITY, "swapped edges");

    cube_reset(&cube);
    cube.cp[0] = 1;
    CHECK(cube_validate(&cube, message, sizeof message) == CUBE_ERR_DUPLICATE_CORNER,
          "duplicate corner");

    cube_reset(&cube);
    cube_to_stickers(&cube, stickers);
    stickers[0] = 'R';      /* Ten R stickers, eight U stickers */
    CHECK(cube_from_stickers(stickers, &parsed, message, sizeof message) == CUBE_ERR_COLOR_COUNT,
          "wrong colour count");

    CHECK(cube_from_stickers("UUU", &parsed, message, sizeof message) == CUBE_ERR_LENGTH,
          "short sticker string");

    cube_reset(&cube);
    cube_to_stickers(&cube, stickers);
    stickers[1] = 'D';      /* Swap two stickers so the UB edge becomes D-B, */
    stickers[28] = 'U';     /* and the DF edge becomes U-F: a second UF edge */
    CHECK(cube_from_stickers(stickers, &parsed, message, sizeof message) != CUBE_OK,
          "impossible edges");

    cube_reset(&cube);
    cube.eo[0] = 1;
    Solution solution;
    CHECK(solver_solve(&cube, &solution) == SOLVE_ERR_INVALID_CUBE, "solver rejects bad cube");
}

/* Solves `cube_count` random cubes and confirms every answer by applying
 * it. Also gathers statistics on solution lengths. */
static void test_solver(unsigned long cube_count)
{
    Cube cube;
    Solution solution;
    unsigned long total_moves = 0;                  /* For the average length      */
    unsigned longest = 0;                           /* Longest solution seen       */
    unsigned longest_phase[NUM_PHASES] = { 0 };     /* Longest run of each phase   */

    cube_reset(&cube);
    CHECK(solver_solve(&cube, &solution) == SOLVE_OK && solution.length == 0,
          "solved cube needs no moves");

    for (unsigned long n = 0; n < cube_count; n++) {
        random_cube(&cube);
        if (solver_solve(&cube, &solution) != SOLVE_OK) {
            CHECK(false, "solver returned an error");
            continue;
        }

        /* The real test: do the moves on a copy and see if it is solved. */
        Cube check = cube;
        cube_apply_moves(&check, solution.moves, solution.length);
        CHECK(cube_is_solved(&check), "solution solves the cube");

        total_moves += solution.length;
        if (solution.length > longest) {
            longest = solution.length;
        }
        for (uint8_t phase = 0; phase < NUM_PHASES; phase++) {
            if (solution.phase_length[phase] > longest_phase[phase]) {
                longest_phase[phase] = solution.phase_length[phase];
            }
        }
    }

    printf("Solved %lu random cubes: average %.2f moves, longest %u moves\n",
           cube_count, cube_count ? (double)total_moves / (double)cube_count : 0.0, longest);
    printf("Longest phases seen: %u + %u + %u + %u\n", longest_phase[0], longest_phase[1],
           longest_phase[2], longest_phase[3]);
}

int main(int argc, char **argv)
{
    /* An optional argument overrides how many random cubes are solved. */
    unsigned long cube_count = DEFAULT_RANDOM_CUBES;
    if (argc > 1) {
        cube_count = strtoul(argv[1], NULL, 10);
    }

    srand(RANDOM_SEED);
    test_moves();
    test_validation();
    test_solver(cube_count);

    if (failures == 0) {
        printf("All tests passed\n");
        return EXIT_SUCCESS;
    }
    printf("%d check(s) failed\n", failures);
    return EXIT_FAILURE;
}
