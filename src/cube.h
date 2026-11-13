/*
 * cube.h
 *
 * Cube representation: a permutation array and an orientation array for the
 * 8 corners and the 12 edges. Every face turn is a table lookup.
 *
 * Piece numbering (a piece is named by the faces it touches):
 *   Corners: 0 URF, 1 UFL, 2 ULB, 3 UBR, 4 DFR, 5 DLF, 6 DBL, 7 DRB
 *   Edges:   0 UR, 1 UF, 2 UL, 3 UB, 4 DR, 5 DF, 6 DL, 7 DB,
 *            8 FR, 9 FL, 10 BL, 11 BR
 *
 * Sticker string layout (54 characters, faces in the order U R F D L B,
 * each face read left to right, top to bottom):
 *
 *            U1 U2 U3
 *            U4 U5 U6
 *            U7 U8 U9
 *   L1 L2 L3 F1 F2 F3 R1 R2 R3 B1 B2 B3
 *   L4 L5 L6 F4 F5 F6 R4 R5 R6 B4 B5 B6
 *   L7 L8 L9 F7 F8 F9 R7 R8 R9 B7 B8 B9
 *            D1 D2 D3
 *            D4 D5 D6
 *            D7 D8 D9
 *
 * Any six distinct characters may be used as colours; the six centre
 * stickers define which colour belongs to which face.
 */
#ifndef CUBE_H
#define CUBE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "rubiks_constants.h"

/* Face numbers, in sticker-string order. */
typedef enum { FACE_U, FACE_R, FACE_F, FACE_D, FACE_L, FACE_B } Face;

typedef struct {
    uint8_t cp[NUM_CORNERS];    /* cp[i] = corner piece sitting in corner position i */
    uint8_t co[NUM_CORNERS];    /* co[i] = twist (0..2) of the corner in position i  */
    uint8_t ep[NUM_EDGES];      /* ep[i] = edge piece sitting in edge position i     */
    uint8_t eo[NUM_EDGES];      /* eo[i] = flip (0..1) of the edge in position i     */
} Cube;

/* Result of checking a cube state. */
typedef enum {
    CUBE_OK,                    /* State is a legal, solvable cube                */
    CUBE_ERR_LENGTH,            /* Sticker string is not 54 characters            */
    CUBE_ERR_CENTERS,           /* Two centre stickers share a colour             */
    CUBE_ERR_UNKNOWN_COLOR,     /* A sticker's colour matches no centre           */
    CUBE_ERR_COLOR_COUNT,       /* A colour does not appear exactly 9 times       */
    CUBE_ERR_BAD_CORNER,        /* A corner's colours match no real corner piece  */
    CUBE_ERR_BAD_EDGE,          /* An edge's colours match no real edge piece     */
    CUBE_ERR_DUPLICATE_CORNER,  /* A corner piece appears more than once          */
    CUBE_ERR_DUPLICATE_EDGE,    /* An edge piece appears more than once           */
    CUBE_ERR_TWIST,             /* Corner twist sum is not a multiple of 3        */
    CUBE_ERR_FLIP,              /* Edge flip sum is not even                      */
    CUBE_ERR_PARITY             /* Corner and edge permutation parities differ    */
} CubeStatus;

/* Basic operations. */
void cube_reset(Cube *cube);
bool cube_is_solved(const Cube *cube);
bool cube_equal(const Cube *a, const Cube *b);
void cube_apply_move(Cube *cube, uint8_t move);
void cube_apply_moves(Cube *cube, const uint8_t *moves, size_t count);

/* Number of position swaps (mod 2) needed to sort a permutation. */
uint8_t permutation_parity(const uint8_t *perm, uint8_t length);

/* Moves. */
uint8_t     move_make(uint8_t face, uint8_t quarter_turns);
uint8_t     move_face(uint8_t move);
uint8_t     move_quarter_turns(uint8_t move);
uint8_t     move_inverse(uint8_t move);
const char *move_name(uint8_t move);

/* Parses "R U2 F'" style text. Returns the number of moves, or -1 on error. */
int moves_parse(const char *text, uint8_t *moves, size_t max_moves);

/* Sticker strings and validation. `message` receives a readable explanation. */
CubeStatus cube_validate(const Cube *cube, char *message, size_t message_size);
CubeStatus cube_from_stickers(const char *stickers, Cube *cube,
                              char *message, size_t message_size);
void       cube_to_stickers(const Cube *cube, char stickers[NUM_STICKERS + 1]);

#endif /* CUBE_H */
