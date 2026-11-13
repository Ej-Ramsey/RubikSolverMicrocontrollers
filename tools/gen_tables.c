/*
 * gen_tables.c  (runs on the PC, not on the Pico)
 *
 * Builds the Thistlethwaite lookup tables with breadth-first searches and
 * writes them as const arrays to a C header (src/thistle_tables.h).
 *
 * Usage: gen_tables <output header path>
 *
 * For every phase the search starts at the phase goal (distance 0) and
 * spreads outwards using only that phase's moves, so each table entry ends
 * up holding the fewest moves needed to reach the goal from that state.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coords.h"
#include "cube.h"
#include "rubiks_constants.h"

/* ------------------------------------------------------------------ */
/* Constants                                                          */
/* ------------------------------------------------------------------ */
#define BYTES_PER_LINE      24      /* Table bytes written per line of the header        */
#define WORDS_PER_LINE      12      /* 16-bit table values written per line              */
#define E_COMB_MASK_COUNT   (1u << NUM_EDGES)       /* Bit masks over 12 edge positions  */
#define M_COMB_MASK_COUNT   (1u << NUM_UD_EDGES)    /* Bit masks over 8 edge positions   */
#define HALF_TURN_COUNT     NUM_FACES               /* One half turn per face            */
#define COSET_UNASSIGNED    0xFFFF  /* Corner permutation not yet given a class          */

/* ------------------------------------------------------------------ */
/* Tables being built                                                 */
/* ------------------------------------------------------------------ */
static uint8_t  dist1[PHASE1_STATE_COUNT];          /* Distances, one byte per state     */
static uint8_t  dist2[PHASE2_STATE_COUNT];
static uint8_t  dist3[PHASE3_STATE_COUNT];
static uint8_t  dist4[PHASE4_STATE_COUNT];
static uint16_t corner_coset[CORNER_PERM_COUNT];    /* Corner permutation -> class id    */
static uint16_t coset_rep[CORNER_COSET_COUNT];      /* One corner permutation per class  */
static uint16_t g3_corners[G3_CORNER_COUNT];        /* Half-turn corner permutations     */
static uint16_t e_comb_mask[E_SLICE_COMB_COUNT];    /* Combination rank -> position mask */
static uint16_t m_comb_mask[M_SLICE_COMB_COUNT];

/* Each phase supplies two functions to the search:
 *   UnrankFn builds some cube that has a given table index, and
 *   RankFn   measures the table index of a cube (the same function the
 *            solver uses on the Pico).                                  */
typedef void     (*UnrankFn)(uint32_t index, Cube *cube);
typedef uint32_t (*RankFn)(const Cube *cube);

/* ------------------------------------------------------------------ */
/* Helpers                                                            */
/* ------------------------------------------------------------------ */

/* Prints an error and stops. Every self-check in this file ends up here,
 * so a wrong table can never be written out silently. */
static void fail(const char *what)
{
    fprintf(stderr, "gen_tables: %s\n", what);
    exit(EXIT_FAILURE);
}

/* Number of 1 bits in `value`. */
static unsigned bit_count(unsigned value)
{
    unsigned count = 0;
    for (; value != 0; value >>= 1) {
        count += value & 1u;
    }
    return count;
}

/* Writes the permutation of 0..length-1 that has the given rank. This is
 * the reverse of permutation_rank() in coords.c. */
static void permutation_unrank(uint16_t rank, uint8_t *perm, uint8_t length)
{
    uint8_t available[NUM_CORNERS];     /* Items not placed yet, in order */
    uint16_t factorial = 1;

    for (uint8_t i = 0; i < length; i++) {
        available[i] = i;
    }

    /* factorial = (length - 1)!, the number of permutations that share
     * the same first item. */
    for (uint8_t i = 2; i < length; i++) {
        factorial = (uint16_t)(factorial * i);
    }

    for (uint8_t i = 0; i < length; i++) {
        /* The rank divided by that count says which unused item comes
         * next; the remainder is the rank among the items that are left. */
        uint8_t pick = (uint8_t)(rank / factorial);
        rank = (uint16_t)(rank % factorial);
        perm[i] = available[pick];

        /* Remove the item just used and shrink the factorial by one step. */
        memmove(&available[pick], &available[pick + 1], (size_t)(length - 1 - i - pick));
        if (length - 1 - i > 0) {
            factorial = (uint16_t)(factorial / (length - 1 - i));
        }
    }
}

/* Puts the edges of one slice in the order with the given rank. */
static void slice_unrank(Cube *cube, const uint8_t positions[SLICE_SIZE], uint8_t rank)
{
    uint8_t order[SLICE_SIZE];
    permutation_unrank(rank, order, SLICE_SIZE);
    for (uint8_t i = 0; i < SLICE_SIZE; i++) {
        cube->ep[positions[i]] = positions[order[i]];
    }
}

/* ------------------------------------------------------------------ */
/* Index -> example cube, and cube -> index, for each phase           */
/* ------------------------------------------------------------------ */

/* Phase 1: solved cube with the edge flips given by the index's bits. */
static void unrank1(uint32_t index, Cube *cube)
{
    uint8_t sum = 0;
    cube_reset(cube);

    /* Peel off one binary digit per edge, last digit first. */
    for (int i = NUM_EDGES - 2; i >= 0; i--) {
        cube->eo[i] = (uint8_t)(index % EDGE_ORIENTATIONS);
        index /= EDGE_ORIENTATIONS;
        sum = (uint8_t)(sum + cube->eo[i]);
    }
    /* The twelfth edge is flipped if needed to make the total even. */
    cube->eo[NUM_EDGES - 1] = (uint8_t)(sum % EDGE_ORIENTATIONS);
}

/* Phase 2: corner twists from the upper part of the index, E-slice edge
 * locations from the lower part. */
static void unrank2(uint32_t index, Cube *cube)
{
    uint16_t mask = e_comb_mask[index % E_SLICE_COMB_COUNT];
    uint32_t twist = index / E_SLICE_COMB_COUNT;
    uint8_t sum = 0;
    uint8_t slice_piece = FIRST_E_SLICE_EDGE;
    uint8_t other_piece = 0;

    cube_reset(cube);

    /* Peel off one base-3 digit per corner, last digit first. */
    for (int i = NUM_CORNERS - 2; i >= 0; i--) {
        cube->co[i] = (uint8_t)(twist % CORNER_ORIENTATIONS);
        twist /= CORNER_ORIENTATIONS;
        sum = (uint8_t)(sum + cube->co[i]);
    }

    /* The eighth corner gets whatever twist makes the total a multiple of 3. */
    cube->co[NUM_CORNERS - 1] =
        (uint8_t)((CORNER_ORIENTATIONS - sum % CORNER_ORIENTATIONS) % CORNER_ORIENTATIONS);

    /* Put E-slice edges in the positions the mask marks and the other
     * edges, in order, everywhere else. */
    for (uint8_t i = 0; i < NUM_EDGES; i++) {
        cube->ep[i] = (mask & (1u << i)) ? slice_piece++ : other_piece++;
    }
}

/* Phase 3: one corner arrangement from the wanted class, and M-slice
 * edges in the positions the mask marks (S-slice edges in the rest). */
static void unrank3(uint32_t index, Cube *cube)
{
    uint16_t mask = m_comb_mask[index % M_SLICE_COMB_COUNT];
    uint8_t m_next = 0;
    uint8_t s_next = 0;

    cube_reset(cube);
    permutation_unrank(coset_rep[index / M_SLICE_COMB_COUNT], cube->cp, NUM_CORNERS);
    for (uint8_t i = 0; i < NUM_UD_EDGES; i++) {
        cube->ep[i] = (mask & (1u << i)) ? M_SLICE_POSITIONS[m_next++]
                                         : S_SLICE_POSITIONS[s_next++];
    }
}

/* Phase 4: split the index back into its four parts (the reverse of the
 * multiply-and-add in coord_phase4) and build exactly that cube. */
static void unrank4(uint32_t index, Cube *cube)
{
    uint8_t e_half = (uint8_t)(index % SLICE_PERM_HALF_COUNT);
    index /= SLICE_PERM_HALF_COUNT;
    uint8_t s_perm = (uint8_t)(index % SLICE_PERM_COUNT);
    index /= SLICE_PERM_COUNT;
    uint8_t m_perm = (uint8_t)(index % SLICE_PERM_COUNT);
    index /= SLICE_PERM_COUNT;

    cube_reset(cube);
    permutation_unrank(g3_corners[index], cube->cp, NUM_CORNERS);
    slice_unrank(cube, M_SLICE_POSITIONS, m_perm);
    slice_unrank(cube, S_SLICE_POSITIONS, s_perm);

    /* Pick whichever of the two E-slice orderings gives a legal cube. */
    slice_unrank(cube, E_SLICE_POSITIONS, (uint8_t)(e_half * 2));
    if (permutation_parity(cube->ep, NUM_EDGES) != permutation_parity(cube->cp, NUM_CORNERS)) {
        slice_unrank(cube, E_SLICE_POSITIONS, (uint8_t)(e_half * 2 + 1));
    }
}

/* Cube -> index. These wrap the shared coordinate functions so that all
 * four have the same shape; phases 3 and 4 use the helper tables above. */
static uint32_t rank1(const Cube *cube) { return coord_phase1(cube); }
static uint32_t rank2(const Cube *cube) { return coord_phase2(cube); }
static uint32_t rank3(const Cube *cube) { return coord_phase3(cube, corner_coset); }
static uint32_t rank4(const Cube *cube) { return coord_phase4(cube, g3_corners); }

/* ------------------------------------------------------------------ */
/* Breadth-first search                                               */
/* ------------------------------------------------------------------ */

/* Fills dist[] with the distance of every state to the solved cube's
 * state, using the moves of `phase`. Returns the largest distance. */
static uint8_t build_distance_table(uint8_t phase, uint8_t *dist, uint32_t state_count,
                                    UnrankFn unrank, RankFn rank)
{
    Cube cube;
    uint32_t reached = 1;   /* States given a distance so far (the goal is the first) */
    uint8_t depth = 0;      /* Distance currently being expanded                      */

    /* Mark everything unvisited except the goal, which is 0 moves away. */
    memset(dist, DIST_UNVISITED, state_count);
    cube_reset(&cube);
    dist[rank(&cube)] = 0;

    printf("Phase %u (%lu states)\n", (unsigned)phase + 1, (unsigned long)state_count);

    /* One pass per distance: every state at distance `depth` gives its
     * unvisited neighbours the distance depth + 1. */
    for (;;) {
        uint32_t at_depth = 0;  /* States found at this distance      */
        uint32_t added = 0;     /* New states found one move further  */

        for (uint32_t index = 0; index < state_count; index++) {
            if (dist[index] != depth) {
                continue;
            }
            at_depth++;

            /* Build a cube with this index, and check that measuring it
             * gives the same index back. */
            unrank(index, &cube);
            if (rank(&cube) != index) {
                fail("unrank/rank mismatch");
            }

            /* Try every move of this phase. Moves can be undone within the
             * phase, so a neighbour is one move further from the goal. */
            for (uint8_t i = 0; i < PHASE_MOVE_COUNT[phase]; i++) {
                Cube next = cube;
                cube_apply_move(&next, PHASE_MOVES[phase][i]);
                uint32_t next_index = rank(&next);
                if (next_index >= state_count) {
                    fail("move left the phase's state space");
                }
                if (dist[next_index] == DIST_UNVISITED) {
                    dist[next_index] = (uint8_t)(depth + 1);
                    added++;
                }
            }
        }
        printf("  distance %2u: %lu states\n", (unsigned)depth, (unsigned long)at_depth);

        /* Nothing new was found, so the search is complete. */
        if (added == 0) {
            break;
        }
        reached += added;
        depth++;

        /* Distances are stored in 4 bits, so they must not exceed 15. */
        if (depth > DIST_MAX) {
            fail("distance does not fit in a table entry");
        }
    }

    /* Every index must have been reached, or the table would have holes. */
    if (reached != state_count) {
        fail("search did not reach every state");
    }
    return depth;
}

/* ------------------------------------------------------------------ */
/* Helper tables                                                      */
/* ------------------------------------------------------------------ */

/* Lists the position masks for every E-slice and M-slice combination.
 * It tries every bit mask, keeps those with exactly four bits set, and
 * files each one under its combination rank. */
static void build_combination_masks(void)
{
    for (unsigned mask = 0; mask < E_COMB_MASK_COUNT; mask++) {
        if (bit_count(mask) == SLICE_SIZE) {
            e_comb_mask[combination_rank((uint16_t)mask, NUM_EDGES)] = (uint16_t)mask;
        }
    }
    for (unsigned mask = 0; mask < M_COMB_MASK_COUNT; mask++) {
        if (bit_count(mask) == SLICE_SIZE) {
            m_comb_mask[combination_rank((uint16_t)mask, NUM_UD_EDGES)] = (uint16_t)mask;
        }
    }
}

/* Finds the 96 corner permutations reachable with half turns, then groups
 * all 8! corner permutations into 420 classes. Two permutations share a
 * class when one is the other preceded by half turns, which is exactly when
 * they need the same moves to reach a half-turn-only state. */
static void build_corner_cosets(void)
{
    static uint8_t in_g3[CORNER_PERM_COUNT];        /* 1 if reachable by half turns   */
    uint8_t g3_perm[G3_CORNER_COUNT][NUM_CORNERS];  /* The reachable arrangements     */
    unsigned found = 1;                             /* How many are in g3_perm so far */
    Cube cube;

    /* Flood fill from the solved corners using the six half turns. The
     * list grows while it is being walked, so every new arrangement gets
     * its own turn at being expanded. */
    cube_reset(&cube);
    memcpy(g3_perm[0], cube.cp, NUM_CORNERS);
    in_g3[coord_corner_perm(&cube)] = 1;
    for (unsigned i = 0; i < found; i++) {
        for (uint8_t face = 0; face < HALF_TURN_COUNT; face++) {
            cube_reset(&cube);
            memcpy(cube.cp, g3_perm[i], NUM_CORNERS);
            cube_apply_move(&cube, move_make(face, 2));
            uint16_t r = coord_corner_perm(&cube);
            if (!in_g3[r]) {
                if (found >= G3_CORNER_COUNT) {
                    fail("too many half-turn corner permutations");
                }
                in_g3[r] = 1;
                memcpy(g3_perm[found++], cube.cp, NUM_CORNERS);
            }
        }
    }
    if (found != G3_CORNER_COUNT) {
        fail("wrong number of half-turn corner permutations");
    }

    /* Sorted list of their ranks, used as the phase 4 corner index. */
    unsigned listed = 0;
    for (uint32_t r = 0; r < CORNER_PERM_COUNT; r++) {
        if (in_g3[r]) {
            g3_corners[listed++] = (uint16_t)r;
        }
    }

    /* Assign class ids in order of the smallest member. */
    unsigned classes = 0;
    for (uint32_t r = 0; r < CORNER_PERM_COUNT; r++) {
        corner_coset[r] = COSET_UNASSIGNED;
    }
    for (uint32_t r = 0; r < CORNER_PERM_COUNT; r++) {
        uint8_t perm[NUM_CORNERS];
        uint8_t member[NUM_CORNERS];

        if (corner_coset[r] != COSET_UNASSIGNED) {
            continue;
        }
        if (classes >= CORNER_COSET_COUNT) {
            fail("too many corner classes");
        }
        /* This arrangement starts a new class. Its 96 members are found by
         * doing each half-turn arrangement first and this one after it,
         * which works out to relabelling the pieces through g3_perm[h]. */
        permutation_unrank((uint16_t)r, perm, NUM_CORNERS);
        for (unsigned h = 0; h < G3_CORNER_COUNT; h++) {
            for (uint8_t i = 0; i < NUM_CORNERS; i++) {
                member[i] = g3_perm[h][perm[i]];
            }
            corner_coset[permutation_rank(member, NUM_CORNERS)] = (uint16_t)classes;
        }
        coset_rep[classes++] = (uint16_t)r;
    }
    if (classes != CORNER_COSET_COUNT) {
        fail("wrong number of corner classes");
    }
    printf("Corner classes: %u, half-turn corner permutations: %u\n", classes, found);
}

/* ------------------------------------------------------------------ */
/* Header output                                                      */
/* ------------------------------------------------------------------ */

/* Writes one distance table, packed DIST_PER_BYTE entries per byte. */
static void write_packed_table(FILE *out, const char *name, const char *size_name,
                               const uint8_t *dist, uint32_t state_count)
{
    uint32_t bytes = (state_count + DIST_PER_BYTE - 1) / DIST_PER_BYTE;

    fprintf(out, "static const uint8_t %s[%s] = {", name, size_name);
    for (uint32_t b = 0; b < bytes; b++) {
        /* Combine two 4-bit distances into one byte: the even index in
         * the low half, the odd index in the high half. */
        unsigned packed = 0;
        for (uint32_t n = 0; n < DIST_PER_BYTE; n++) {
            uint32_t index = b * DIST_PER_BYTE + n;
            if (index < state_count) {
                packed |= (unsigned)(dist[index] & DIST_MASK) << (n * DIST_BITS);
            }
        }
        fprintf(out, "%s0x%02X,", (b % BYTES_PER_LINE == 0) ? "\n    " : "", packed);
    }
    fprintf(out, "\n};\n\n");
}

/* Writes a table of 16-bit values as plain decimal numbers. */
static void write_word_table(FILE *out, const char *name, const char *size_name,
                             const uint16_t *values, uint32_t count)
{
    fprintf(out, "static const uint16_t %s[%s] = {", name, size_name);
    for (uint32_t i = 0; i < count; i++) {
        fprintf(out, "%s%u,", (i % WORDS_PER_LINE == 0) ? "\n    " : "", (unsigned)values[i]);
    }
    fprintf(out, "\n};\n\n");
}

int main(int argc, char **argv)
{
    uint8_t max_depth[NUM_PHASES];

    if (argc != 2) {
        fprintf(stderr, "usage: %s <output header>\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* The helper tables come first because phases 2-4 need them, then one
     * breadth-first search per phase. */
    build_combination_masks();
    build_corner_cosets();
    max_depth[0] = build_distance_table(0, dist1, PHASE1_STATE_COUNT, unrank1, rank1);
    max_depth[1] = build_distance_table(1, dist2, PHASE2_STATE_COUNT, unrank2, rank2);
    max_depth[2] = build_distance_table(2, dist3, PHASE3_STATE_COUNT, unrank3, rank3);
    max_depth[3] = build_distance_table(3, dist4, PHASE4_STATE_COUNT, unrank4, rank4);

    /* Write everything out as a C header. */
    FILE *out = fopen(argv[1], "w");
    if (out == NULL) {
        fail("cannot open output file");
    }

    /* File comment, include guard and includes. */
    fprintf(out,
            "/*\n"
            " * thistle_tables.h\n"
            " *\n"
            " * GENERATED FILE - do not edit. Produced by tools/gen_tables.c.\n"
            " *\n"
            " * Thistlethwaite lookup tables. They are const, so on the Pico they are\n"
            " * stored in flash and read in place (XIP) without using any RAM.\n"
            " * Include this file from solver.c only.\n"
            " */\n"
            "#ifndef THISTLE_TABLES_H\n"
            "#define THISTLE_TABLES_H\n\n"
            "#include <stdint.h>\n\n"
            "#include \"rubiks_constants.h\"\n\n");

    fprintf(out, "/* Longest move sequence each phase can need. */\n");
    for (uint8_t phase = 0; phase < NUM_PHASES; phase++) {
        fprintf(out, "#define PHASE%u_MAX_DEPTH %u\n", (unsigned)phase + 1,
                (unsigned)max_depth[phase]);
    }

    fprintf(out, "\n/* Phase 1: moves needed to fix every edge flip. */\n");
    write_packed_table(out, "PHASE1_DIST", "PHASE1_TABLE_BYTES", dist1, PHASE1_STATE_COUNT);
    fprintf(out, "/* Phase 2: moves needed to untwist corners and fill the E slice. */\n");
    write_packed_table(out, "PHASE2_DIST", "PHASE2_TABLE_BYTES", dist2, PHASE2_STATE_COUNT);
    fprintf(out, "/* Phase 3: moves needed to reach a half-turn-only state. */\n");
    write_packed_table(out, "PHASE3_DIST", "PHASE3_TABLE_BYTES", dist3, PHASE3_STATE_COUNT);
    fprintf(out, "/* Phase 4: half turns needed to solve the cube. */\n");
    write_packed_table(out, "PHASE4_DIST", "PHASE4_TABLE_BYTES", dist4, PHASE4_STATE_COUNT);
    fprintf(out, "/* Phase 3 helper: class of every corner permutation. */\n");
    write_word_table(out, "CORNER_COSET", "CORNER_PERM_COUNT", corner_coset, CORNER_PERM_COUNT);
    fprintf(out, "/* Phase 4 helper: sorted ranks of the half-turn corner permutations. */\n");
    write_word_table(out, "G3_CORNERS", "G3_CORNER_COUNT", g3_corners, G3_CORNER_COUNT);
    fprintf(out, "#endif /* THISTLE_TABLES_H */\n");

    if (fclose(out) != 0) {
        fail("error writing output file");
    }

    printf("Longest phases: %u + %u + %u + %u = %u moves\n", max_depth[0], max_depth[1],
           max_depth[2], max_depth[3],
           max_depth[0] + max_depth[1] + max_depth[2] + max_depth[3]);
    printf("Wrote %s\n", argv[1]);
    return EXIT_SUCCESS;
}
