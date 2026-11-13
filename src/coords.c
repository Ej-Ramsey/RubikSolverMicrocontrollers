/*
 * coords.c
 *
 * Converts a cube into the table index ("coordinate") used by each phase.
 */
#include "coords.h"

/* ------------------------------------------------------------------ */
/* Constants                                                          */
/* ------------------------------------------------------------------ */

/* Move codes: face * TURNS_PER_FACE + (quarter turns - 1). */
enum {
    MOVE_U1, MOVE_U2, MOVE_U3,      /* U, U2, U' */
    MOVE_R1, MOVE_R2, MOVE_R3,      /* R, R2, R' */
    MOVE_F1, MOVE_F2, MOVE_F3,      /* F, F2, F' */
    MOVE_D1, MOVE_D2, MOVE_D3,      /* D, D2, D' */
    MOVE_L1, MOVE_L2, MOVE_L3,      /* L, L2, L' */
    MOVE_B1, MOVE_B2, MOVE_B3       /* B, B2, B' */
};

/* Moves allowed in each phase. Each phase removes quarter turns of one
 * more pair of faces, so progress made by earlier phases cannot be undone. */
const uint8_t PHASE_MOVES[NUM_PHASES][NUM_MOVES] = {
    /* Phase 1: every move */
    { MOVE_U1, MOVE_U2, MOVE_U3, MOVE_D1, MOVE_D2, MOVE_D3, MOVE_R1, MOVE_R2, MOVE_R3,
      MOVE_L1, MOVE_L2, MOVE_L3, MOVE_F1, MOVE_F2, MOVE_F3, MOVE_B1, MOVE_B2, MOVE_B3 },
    /* Phase 2: F and B only as half turns */
    { MOVE_U1, MOVE_U2, MOVE_U3, MOVE_D1, MOVE_D2, MOVE_D3, MOVE_R1, MOVE_R2, MOVE_R3,
      MOVE_L1, MOVE_L2, MOVE_L3, MOVE_F2, MOVE_B2 },
    /* Phase 3: R and L also only as half turns */
    { MOVE_U1, MOVE_U2, MOVE_U3, MOVE_D1, MOVE_D2, MOVE_D3,
      MOVE_R2, MOVE_L2, MOVE_F2, MOVE_B2 },
    /* Phase 4: half turns only */
    { MOVE_U2, MOVE_D2, MOVE_R2, MOVE_L2, MOVE_F2, MOVE_B2 },
};

/* Number of valid entries in each row of PHASE_MOVES. */
const uint8_t PHASE_MOVE_COUNT[NUM_PHASES] = {
    PHASE1_MOVE_COUNT, PHASE2_MOVE_COUNT, PHASE3_MOVE_COUNT, PHASE4_MOVE_COUNT
};

/* Edge positions that make up each middle slice. */
const uint8_t M_SLICE_POSITIONS[SLICE_SIZE] = { 1, 3, 5, 7 };      /* UF, UB, DF, DB */
const uint8_t S_SLICE_POSITIONS[SLICE_SIZE] = { 0, 2, 4, 6 };      /* UR, UL, DR, DL */
const uint8_t E_SLICE_POSITIONS[SLICE_SIZE] = { 8, 9, 10, 11 };    /* FR, FL, BL, BR */

/* BINOMIAL[n][k] = number of ways to choose k items from n (k <= SLICE_SIZE). */
static const uint16_t BINOMIAL[NUM_EDGES + 1][SLICE_SIZE + 1] = {
    { 1, 0, 0, 0, 0 },      { 1, 1, 0, 0, 0 },      { 1, 2, 1, 0, 0 },
    { 1, 3, 3, 1, 0 },      { 1, 4, 6, 4, 1 },      { 1, 5, 10, 10, 5 },
    { 1, 6, 15, 20, 15 },   { 1, 7, 21, 35, 35 },   { 1, 8, 28, 56, 70 },
    { 1, 9, 36, 84, 126 },  { 1, 10, 45, 120, 210 }, { 1, 11, 55, 165, 330 },
    { 1, 12, 66, 220, 495 },
};

/* FACTORIAL[n] = n!, for ranking permutations of up to NUM_CORNERS items. */
static const uint16_t FACTORIAL[NUM_CORNERS + 1] = { 1, 1, 2, 6, 24, 120, 720, 5040, 40320 };

/* ------------------------------------------------------------------ */
/* Ranking helpers                                                    */
/* ------------------------------------------------------------------ */

/* Numbers a permutation by its place in dictionary order, so the sorted
 * permutation is 0 and the fully reversed one is length! - 1.
 * For each item, count how many smaller items come after it; that count
 * times the number of ways to arrange the remaining items is how many
 * permutations come before this one because of that item. */
uint16_t permutation_rank(const uint8_t *perm, uint8_t length)
{
    uint16_t rank = 0;
    for (uint8_t i = 0; i < length; i++) {
        uint8_t smaller_after = 0;
        for (uint8_t j = (uint8_t)(i + 1); j < length; j++) {
            if (perm[j] < perm[i]) {
                smaller_after++;
            }
        }
        rank = (uint16_t)(rank + smaller_after * FACTORIAL[length - 1 - i]);
    }
    return rank;
}

/* Numbers a choice of SLICE_SIZE positions out of `positions` with a
 * unique value from 0 to C(positions, SLICE_SIZE) - 1. The k-th chosen
 * position (counting from 1) adds C(position, k) to the rank. */
uint16_t combination_rank(uint16_t mask, uint8_t positions)
{
    uint16_t rank = 0;
    uint8_t chosen = 0;     /* How many chosen positions have been seen so far */
    for (uint8_t pos = 0; pos < positions; pos++) {
        if (mask & (1u << pos)) {
            chosen++;
            if (chosen <= SLICE_SIZE) {
                rank = (uint16_t)(rank + BINOMIAL[pos][chosen]);
            }
        }
    }
    return rank;
}

/* ------------------------------------------------------------------ */
/* Building blocks                                                    */
/* ------------------------------------------------------------------ */

/* Edge flips read as an 11-digit binary number. Zero means every edge is
 * flipped correctly, which is the goal of phase 1. */
uint16_t coord_edge_orient(const Cube *cube)
{
    /* The last edge's flip is fixed by the others, so it is left out. */
    uint16_t value = 0;
    for (uint8_t i = 0; i < NUM_EDGES - 1; i++) {
        value = (uint16_t)(value * EDGE_ORIENTATIONS + cube->eo[i]);
    }
    return value;
}

/* Corner twists read as a 7-digit base-3 number. Zero means no corner is
 * twisted, which is half of the goal of phase 2. */
uint16_t coord_corner_orient(const Cube *cube)
{
    /* The last corner's twist is fixed by the others, so it is left out. */
    uint16_t value = 0;
    for (uint8_t i = 0; i < NUM_CORNERS - 1; i++) {
        value = (uint16_t)(value * CORNER_ORIENTATIONS + cube->co[i]);
    }
    return value;
}

/* Where the four E-slice edges are, ignoring their order. The other half
 * of the phase 2 goal is to bring all four back into the E slice. */
uint16_t coord_e_slice_comb(const Cube *cube)
{
    /* Which of the 12 edge positions hold the four E-slice edges. */
    uint16_t mask = 0;
    for (uint8_t i = 0; i < NUM_EDGES; i++) {
        if (cube->ep[i] >= FIRST_E_SLICE_EDGE) {
            mask |= (uint16_t)(1u << i);
        }
    }
    return combination_rank(mask, NUM_EDGES);
}

/* Where the four M-slice edges are, ignoring their order. Used by phase 3,
 * when the E slice is already filled, so only 8 positions are left. */
uint16_t coord_m_slice_comb(const Cube *cube)
{
    /* Which of the 8 U/D-layer positions hold the four M-slice edges.
     * M-slice edges are the odd-numbered ones below FIRST_E_SLICE_EDGE. */
    uint16_t mask = 0;
    for (uint8_t i = 0; i < NUM_UD_EDGES; i++) {
        if (cube->ep[i] < FIRST_E_SLICE_EDGE && (cube->ep[i] & 1u)) {
            mask |= (uint16_t)(1u << i);
        }
    }
    return combination_rank(mask, NUM_UD_EDGES);
}

/* Which of the 8! arrangements the corners are in (twists ignored). */
uint16_t coord_corner_perm(const Cube *cube)
{
    return permutation_rank(cube->cp, NUM_CORNERS);
}

/* Order of the four edges inside one slice, as a number 0..23. */
uint8_t coord_slice_perm(const Cube *cube, const uint8_t positions[SLICE_SIZE])
{
    uint8_t order[SLICE_SIZE];  /* The slice's edges renumbered 0..3 */

    for (uint8_t i = 0; i < SLICE_SIZE; i++) {
        /* Find which of the slice's four home positions this piece belongs
         * to (a piece's number is the number of its home position). */
        uint8_t piece = cube->ep[positions[i]];
        uint8_t home = 0;
        while (home < SLICE_SIZE && positions[home] != piece) {
            home++;
        }
        if (home == SLICE_SIZE) {
            return SLICE_PERM_COUNT;    /* An edge from another slice is here */
        }
        order[i] = home;
    }
    return (uint8_t)permutation_rank(order, SLICE_SIZE);
}

/* ------------------------------------------------------------------ */
/* Phase table indices                                                */
/* ------------------------------------------------------------------ */

/* When a phase measures two features, they are combined into one index the
 * same way row and column are combined for a 2-D array:
 *     index = first * (number of values of second) + second            */

/* Phase 1 index: the edge flip pattern. */
uint32_t coord_phase1(const Cube *cube)
{
    return coord_edge_orient(cube);
}

/* Phase 2 index: corner twist pattern and E-slice edge locations. */
uint32_t coord_phase2(const Cube *cube)
{
    return (uint32_t)coord_corner_orient(cube) * E_SLICE_COMB_COUNT + coord_e_slice_comb(cube);
}

/* Phase 3 index: corner permutation class and M-slice edge locations.
 * The class comes from a lookup table because it has no simple formula:
 * it groups together corner arrangements that differ only by half turns,
 * since those need exactly the same moves to finish the phase. */
uint32_t coord_phase3(const Cube *cube, const uint16_t *corner_coset)
{
    return (uint32_t)corner_coset[coord_corner_perm(cube)] * M_SLICE_COMB_COUNT +
           coord_m_slice_comb(cube);
}

/* Phase 4 index: the exact corner arrangement (one of only 96 that half
 * turns can produce) and the order of the edges inside each slice. This
 * identifies the cube completely, so a table distance of 0 means solved. */
uint32_t coord_phase4(const Cube *cube, const uint16_t *g3_corners)
{
    /* Binary search for the corner permutation among the 96 legal ones. */
    uint16_t rank = coord_corner_perm(cube);
    int low = 0;
    int high = G3_CORNER_COUNT - 1;
    int corner_index = -1;
    while (low <= high) {
        int middle = (low + high) / 2;
        if (g3_corners[middle] == rank) {
            corner_index = middle;
            break;
        }
        if (g3_corners[middle] < rank) {
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    }

    /* Order of the edges inside each of the three slices. */
    uint8_t m_perm = coord_slice_perm(cube, M_SLICE_POSITIONS);
    uint8_t s_perm = coord_slice_perm(cube, S_SLICE_POSITIONS);
    uint8_t e_perm = coord_slice_perm(cube, E_SLICE_POSITIONS);

    /* If any part is not a half-turn-only arrangement, phase 3 was not
     * finished and the cube has no entry in the phase 4 table. */
    if (corner_index < 0 || m_perm == SLICE_PERM_COUNT ||
        s_perm == SLICE_PERM_COUNT || e_perm == SLICE_PERM_COUNT) {
        return COORD_INVALID;
    }

    /* Permutation parity fixes the last bit of the E-slice ordering, so
     * only half of it (e_perm / 2) needs to be stored. */
    uint32_t index = (uint32_t)corner_index;
    index = index * SLICE_PERM_COUNT + m_perm;
    index = index * SLICE_PERM_COUNT + s_perm;
    index = index * SLICE_PERM_HALF_COUNT + (uint32_t)(e_perm / 2);
    return index;
}
