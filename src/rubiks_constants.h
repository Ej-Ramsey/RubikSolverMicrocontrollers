/*
 * rubiks_constants.h
 *
 * Every shared constant of the Rubik's Solver lives here, each with a
 * description, so that no magic numbers appear in the rest of the program.
 */
#ifndef RUBIKS_CONSTANTS_H
#define RUBIKS_CONSTANTS_H

/* ------------------------------------------------------------------ */
/* Cube geometry                                                      */
/* ------------------------------------------------------------------ */
#define NUM_FACES               6   /* Faces on the cube: U, R, F, D, L, B              */
#define NUM_CORNERS             8   /* Corner pieces (3 stickers each)                  */
#define NUM_EDGES               12  /* Edge pieces (2 stickers each)                    */
#define STICKERS_PER_FACE       9   /* Stickers on one face (3 x 3)                     */
#define NUM_STICKERS            54  /* Stickers on the whole cube (6 faces x 9)         */
#define CENTER_STICKER_OFFSET   4   /* Index of the centre sticker within a face (0..8) */
#define STICKERS_PER_CORNER     3   /* Stickers on one corner piece                     */
#define STICKERS_PER_EDGE       2   /* Stickers on one edge piece                       */
#define CORNER_ORIENTATIONS     3   /* Ways a corner can be twisted in its position     */
#define EDGE_ORIENTATIONS       2   /* Ways an edge can be flipped in its position      */
#define SLICE_SIZE              4   /* Edges in one middle slice (M, S or E)            */
#define FIRST_E_SLICE_EDGE      8   /* Edges 8..11 (FR, FL, BL, BR) form the E slice    */
#define NUM_UD_EDGES            8   /* Edges 0..7 lie in the U and D layers             */

/* ------------------------------------------------------------------ */
/* Moves                                                              */
/* A move is stored as one byte: face * TURNS_PER_FACE + (turns - 1), */
/* where turns is the number of clockwise quarter turns (1, 2 or 3).  */
/* ------------------------------------------------------------------ */
#define TURNS_PER_FACE          3   /* Distinct turns of one face: 90 deg, 180 deg, 270 deg */
#define NUM_MOVES               18  /* Total distinct moves (6 faces x 3 turns)             */
#define QUARTER_TURNS_PER_CYCLE 4   /* Quarter turns that bring a face back to its start    */
#define HALF_TURN_INDEX         1   /* (turns - 1) value of a 180 degree turn               */
#define MOVE_NONE               0xFF /* Marker for "no move" / an unparsable move           */
#define MOVE_NAME_MAX           3   /* Longest move name including terminator, e.g. "R2"    */

/* ------------------------------------------------------------------ */
/* Thistlethwaite phases                                              */
/*   Phase 1: all moves              -> every edge correctly flipped  */
/*   Phase 2: U D L R F2 B2          -> corners untwisted, E slice in */
/*   Phase 3: U D L2 R2 F2 B2        -> pieces in half-turn orbits    */
/*   Phase 4: U2 D2 L2 R2 F2 B2      -> solved                        */
/* ------------------------------------------------------------------ */
#define NUM_PHASES              4   /* Number of solving phases                         */
#define PHASE1_MOVE_COUNT       18  /* Moves allowed in phase 1                         */
#define PHASE2_MOVE_COUNT       14  /* Moves allowed in phase 2                         */
#define PHASE3_MOVE_COUNT       10  /* Moves allowed in phase 3                         */
#define PHASE4_MOVE_COUNT       6   /* Moves allowed in phase 4                         */

/* ------------------------------------------------------------------ */
/* Coordinate sizes (how many values each measured feature can take)  */
/* ------------------------------------------------------------------ */
#define EDGE_ORIENT_COUNT       2048    /* 2^11 edge flip patterns (12th flip is forced)       */
#define CORNER_ORIENT_COUNT     2187    /* 3^7 corner twist patterns (8th twist is forced)     */
#define E_SLICE_COMB_COUNT      495     /* C(12,4) places the 4 E-slice edges can occupy       */
#define CORNER_PERM_COUNT       40320   /* 8! corner permutations                              */
#define CORNER_COSET_COUNT      420     /* 8!/96 corner permutation classes used by phase 3    */
#define M_SLICE_COMB_COUNT      70      /* C(8,4) places the 4 M-slice edges can occupy        */
#define G3_CORNER_COUNT         96      /* Corner permutations reachable with half turns only  */
#define SLICE_PERM_COUNT        24      /* 4! orderings of the edges inside one slice          */
#define SLICE_PERM_HALF_COUNT   12      /* 4!/2: last slice's ordering once parity is known    */

/* Number of states in each phase's lookup table. */
#define PHASE1_STATE_COUNT      EDGE_ORIENT_COUNT                               /* 2,048     */
#define PHASE2_STATE_COUNT      (CORNER_ORIENT_COUNT * E_SLICE_COMB_COUNT)      /* 1,082,565 */
#define PHASE3_STATE_COUNT      (CORNER_COSET_COUNT * M_SLICE_COMB_COUNT)       /* 29,400    */
#define PHASE4_STATE_COUNT      (G3_CORNER_COUNT * SLICE_PERM_COUNT * \
                                 SLICE_PERM_COUNT * SLICE_PERM_HALF_COUNT)      /* 663,552   */

#define COORD_INVALID           0xFFFFFFFFu /* Returned when a cube is not in the phase's start group */

/* ------------------------------------------------------------------ */
/* Lookup table storage                                               */
/* Each entry is the number of moves to the phase goal, packed two    */
/* entries per byte so the tables use as little flash as possible.    */
/* ------------------------------------------------------------------ */
#define DIST_BITS               4       /* Bits used by one distance entry                     */
#define DIST_MASK               0x0F    /* Mask that isolates one distance entry               */
#define DIST_PER_BYTE           2       /* Distance entries packed into one byte               */
#define DIST_MAX                15      /* Largest distance that fits in one entry             */
#define DIST_UNVISITED          0xFF    /* Generator only: state not reached by the search yet */

/* Bytes of flash used by each packed distance table (rounded up). */
#define PHASE1_TABLE_BYTES      ((PHASE1_STATE_COUNT + DIST_PER_BYTE - 1) / DIST_PER_BYTE)
#define PHASE2_TABLE_BYTES      ((PHASE2_STATE_COUNT + DIST_PER_BYTE - 1) / DIST_PER_BYTE)
#define PHASE3_TABLE_BYTES      ((PHASE3_STATE_COUNT + DIST_PER_BYTE - 1) / DIST_PER_BYTE)
#define PHASE4_TABLE_BYTES      ((PHASE4_STATE_COUNT + DIST_PER_BYTE - 1) / DIST_PER_BYTE)

/* ------------------------------------------------------------------ */
/* Solver output                                                      */
/* ------------------------------------------------------------------ */
#define MAX_SOLUTION_MOVES      64  /* Room for the longest possible solution (worst case 45) */

/* ------------------------------------------------------------------ */
/* Sticker entry (IR remote) and messages (TFT)                       */
/* ------------------------------------------------------------------ */
#define NUM_COLORS              6    /* Sticker colours: white, yellow, red, orange, green, blue */
#define COLOR_UNSET             0xFF /* Sticker colour not entered yet                           */
#define MESSAGE_MAX             96   /* Longest validation / error message                       */

#endif /* RUBIKS_CONSTANTS_H */
