/*
 * coords.h
 *
 * "Coordinates": the numbers each Thistlethwaite phase measures on the cube
 * and uses as an index into its lookup table. This file is shared by the
 * on-device solver and by the PC table generator so that both always agree
 * on how a cube maps to a table index.
 */
#ifndef COORDS_H
#define COORDS_H

#include <stdint.h>

#include "cube.h"

/* Moves each phase is allowed to use, and how many there are. */
extern const uint8_t PHASE_MOVES[NUM_PHASES][NUM_MOVES];
extern const uint8_t PHASE_MOVE_COUNT[NUM_PHASES];

/* Position sets of the three middle slices. */
extern const uint8_t M_SLICE_POSITIONS[SLICE_SIZE];   /* UF, UB, DF, DB */
extern const uint8_t S_SLICE_POSITIONS[SLICE_SIZE];   /* UR, UL, DR, DL */
extern const uint8_t E_SLICE_POSITIONS[SLICE_SIZE];   /* FR, FL, BL, BR */

/* Building blocks. */
uint16_t coord_edge_orient(const Cube *cube);       /* 0 .. EDGE_ORIENT_COUNT - 1   */
uint16_t coord_corner_orient(const Cube *cube);     /* 0 .. CORNER_ORIENT_COUNT - 1 */
uint16_t coord_e_slice_comb(const Cube *cube);      /* 0 .. E_SLICE_COMB_COUNT - 1  */
uint16_t coord_m_slice_comb(const Cube *cube);      /* 0 .. M_SLICE_COMB_COUNT - 1  */
uint16_t coord_corner_perm(const Cube *cube);       /* 0 .. CORNER_PERM_COUNT - 1   */

/* Ordering (0 .. SLICE_PERM_COUNT - 1) of the four edges inside one slice,
 * or SLICE_PERM_COUNT if a foreign edge is in the slice. */
uint8_t coord_slice_perm(const Cube *cube, const uint8_t positions[SLICE_SIZE]);

/* Rank (0 .. length! - 1) of a permutation of 0 .. length - 1. */
uint16_t permutation_rank(const uint8_t *perm, uint8_t length);

/* Rank of a choice of positions given as a bit mask (bit i = position i). */
uint16_t combination_rank(uint16_t mask, uint8_t positions);

/* Table index of the cube for each phase.
 *   corner_coset: CORNER_PERM_COUNT entries, corner permutation -> class id
 *   g3_corners:   G3_CORNER_COUNT sorted corner permutation ranks
 * Phase 4 returns COORD_INVALID if the cube is not a half-turn-only state. */
uint32_t coord_phase1(const Cube *cube);
uint32_t coord_phase2(const Cube *cube);
uint32_t coord_phase3(const Cube *cube, const uint16_t *corner_coset);
uint32_t coord_phase4(const Cube *cube, const uint16_t *g3_corners);

#endif /* COORDS_H */
