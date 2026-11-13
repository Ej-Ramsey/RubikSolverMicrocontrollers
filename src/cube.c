/*
 * cube.c
 *
 * Cube representation, face-turn tables, move text, sticker-string
 * conversion and state validation. See cube.h for the piece numbering.
 */
#include "cube.h"

#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Constants                                                          */
/* ------------------------------------------------------------------ */

/* Sticker index of sticker n (1..9) on each face of the sticker string. */
#define U(n) (FACE_U * STICKERS_PER_FACE + (n) - 1)
#define R(n) (FACE_R * STICKERS_PER_FACE + (n) - 1)
#define F(n) (FACE_F * STICKERS_PER_FACE + (n) - 1)
#define D(n) (FACE_D * STICKERS_PER_FACE + (n) - 1)
#define L(n) (FACE_L * STICKERS_PER_FACE + (n) - 1)
#define B(n) (FACE_B * STICKERS_PER_FACE + (n) - 1)

/* Letter used for each face in move names and generated sticker strings. */
static const char FACE_LETTERS[NUM_FACES + 1] = "URFDLB";

/* Suffix printed after the face letter for 1, 2 and 3 quarter turns. */
static const char *const TURN_SUFFIXES[TURNS_PER_FACE] = { "", "2", "'" };

/* Characters that separate moves in a move string. */
static const char MOVE_SEPARATORS[] = " \t\r\n,";

/* Effect of one clockwise quarter turn of a face. */
typedef struct {
    uint8_t cp[NUM_CORNERS];    /* Position i receives the corner from position cp[i] */
    uint8_t co[NUM_CORNERS];    /* Twist added to the corner arriving at position i   */
    uint8_t ep[NUM_EDGES];      /* Position i receives the edge from position ep[i]   */
    uint8_t eo[NUM_EDGES];      /* Flip added to the edge arriving at position i      */
} FaceTurn;

/* Quarter-turn tables, indexed by Face. Only F and B flip edges, and only
 * R, F, L and B twist corners; the Thistlethwaite phases rely on this. */
static const FaceTurn FACE_TURNS[NUM_FACES] = {
    [FACE_U] = { { 3, 0, 1, 2, 4, 5, 6, 7 }, { 0, 0, 0, 0, 0, 0, 0, 0 },
                 { 3, 0, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11 },
                 { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
    [FACE_R] = { { 4, 1, 2, 0, 7, 5, 6, 3 }, { 2, 0, 0, 1, 1, 0, 0, 2 },
                 { 8, 1, 2, 3, 11, 5, 6, 7, 4, 9, 10, 0 },
                 { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
    [FACE_F] = { { 1, 5, 2, 3, 0, 4, 6, 7 }, { 1, 2, 0, 0, 2, 1, 0, 0 },
                 { 0, 9, 2, 3, 4, 8, 6, 7, 1, 5, 10, 11 },
                 { 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0 } },
    [FACE_D] = { { 0, 1, 2, 3, 5, 6, 7, 4 }, { 0, 0, 0, 0, 0, 0, 0, 0 },
                 { 0, 1, 2, 3, 5, 6, 7, 4, 8, 9, 10, 11 },
                 { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
    [FACE_L] = { { 0, 2, 6, 3, 4, 1, 5, 7 }, { 0, 1, 2, 0, 0, 2, 1, 0 },
                 { 0, 1, 10, 3, 4, 5, 9, 7, 8, 2, 6, 11 },
                 { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
    [FACE_B] = { { 0, 1, 3, 7, 4, 5, 2, 6 }, { 0, 0, 1, 2, 0, 0, 2, 1 },
                 { 0, 1, 2, 11, 4, 5, 6, 10, 8, 9, 3, 7 },
                 { 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1 } },
};

/* Sticker indices of each corner position. The first entry is the U or D
 * sticker; the others follow clockwise around the corner. */
static const uint8_t CORNER_STICKERS[NUM_CORNERS][STICKERS_PER_CORNER] = {
    { U(9), R(1), F(3) }, { U(7), F(1), L(3) }, { U(1), L(1), B(3) }, { U(3), B(1), R(3) },
    { D(3), F(9), R(7) }, { D(1), L(9), F(7) }, { D(7), B(9), L(7) }, { D(9), R(9), B(7) },
};

/* Sticker indices of each edge position. The first entry is the sticker that
 * defines the edge's flip (U/D sticker, or F/B sticker for E-slice edges). */
static const uint8_t EDGE_STICKERS[NUM_EDGES][STICKERS_PER_EDGE] = {
    { U(6), R(2) }, { U(8), F(2) }, { U(4), L(2) }, { U(2), B(2) },
    { D(6), R(8) }, { D(2), F(8) }, { D(4), L(8) }, { D(8), B(8) },
    { F(6), R(4) }, { F(4), L(6) }, { B(6), L(4) }, { B(4), R(6) },
};

/* ------------------------------------------------------------------ */
/* Basic operations                                                   */
/* ------------------------------------------------------------------ */

/* Sets the cube to the solved state: every piece sits in its own home
 * position (piece i in position i) with no twist and no flip. */
void cube_reset(Cube *cube)
{
    for (uint8_t i = 0; i < NUM_CORNERS; i++) {
        cube->cp[i] = i;
        cube->co[i] = 0;
    }
    for (uint8_t i = 0; i < NUM_EDGES; i++) {
        cube->ep[i] = i;
        cube->eo[i] = 0;
    }
}

/* Two cubes are the same when all four of their arrays match. */
bool cube_equal(const Cube *a, const Cube *b)
{
    return memcmp(a->cp, b->cp, NUM_CORNERS) == 0 &&
           memcmp(a->co, b->co, NUM_CORNERS) == 0 &&
           memcmp(a->ep, b->ep, NUM_EDGES) == 0 &&
           memcmp(a->eo, b->eo, NUM_EDGES) == 0;
}

/* A cube is solved when it equals a freshly reset cube. */
bool cube_is_solved(const Cube *cube)
{
    Cube solved;
    cube_reset(&solved);
    return cube_equal(cube, &solved);
}

/* Applies one clockwise quarter turn of `face` using its lookup table. */
static void apply_quarter_turn(Cube *cube, uint8_t face)
{
    const FaceTurn *turn = &FACE_TURNS[face];

    /* Work from a copy, because every position is read while the cube is
     * being overwritten. */
    Cube before = *cube;

    /* Corners: position i receives the piece that was in position
     * turn->cp[i]. Its twist is its old twist plus the twist this turn
     * adds, wrapped around to stay in 0..2. */
    for (uint8_t i = 0; i < NUM_CORNERS; i++) {
        cube->cp[i] = before.cp[turn->cp[i]];
        cube->co[i] = (uint8_t)((before.co[turn->cp[i]] + turn->co[i]) % CORNER_ORIENTATIONS);
    }

    /* Edges: the same idea, with the flip wrapped around to stay in 0..1. */
    for (uint8_t i = 0; i < NUM_EDGES; i++) {
        cube->ep[i] = before.ep[turn->ep[i]];
        cube->eo[i] = (uint8_t)((before.eo[turn->ep[i]] + turn->eo[i]) % EDGE_ORIENTATIONS);
    }
}

/* Applies one move. A half turn or counter-clockwise turn is done as two
 * or three clockwise quarter turns, so only six tables are needed. */
void cube_apply_move(Cube *cube, uint8_t move)
{
    uint8_t face = move_face(move);
    for (uint8_t n = move_quarter_turns(move); n > 0; n--) {
        apply_quarter_turn(cube, face);
    }
}

/* Applies a list of moves in order. */
void cube_apply_moves(Cube *cube, const uint8_t *moves, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        cube_apply_move(cube, moves[i]);
    }
}

/* Returns 0 if the permutation is even and 1 if it is odd. It counts the
 * pairs that are out of order; each such pair toggles the parity. A real
 * cube always has the same parity for its corners and its edges. */
uint8_t permutation_parity(const uint8_t *perm, uint8_t length)
{
    uint8_t parity = 0;
    for (uint8_t i = 0; i < length; i++) {
        for (uint8_t j = (uint8_t)(i + 1); j < length; j++) {
            if (perm[i] > perm[j]) {
                parity ^= 1;
            }
        }
    }
    return parity;
}

/* ------------------------------------------------------------------ */
/* Moves                                                              */
/* ------------------------------------------------------------------ */

/* Packs a face (0..5) and a number of clockwise quarter turns (1..3) into
 * one move byte. */
uint8_t move_make(uint8_t face, uint8_t quarter_turns)
{
    return (uint8_t)(face * TURNS_PER_FACE + quarter_turns - 1);
}

/* Face (0..5) that a move byte turns. */
uint8_t move_face(uint8_t move)
{
    return (uint8_t)(move / TURNS_PER_FACE);
}

/* Number of clockwise quarter turns (1..3) in a move byte. */
uint8_t move_quarter_turns(uint8_t move)
{
    return (uint8_t)(move % TURNS_PER_FACE + 1);
}

/* The move that undoes `move`: same face, turned the rest of the way
 * around (1 <-> 3 quarter turns; a half turn undoes itself). */
uint8_t move_inverse(uint8_t move)
{
    return move_make(move_face(move),
                     (uint8_t)(QUARTER_TURNS_PER_CYCLE - move_quarter_turns(move)));
}

/* Text name of a move, such as "R", "U2" or "F'". */
const char *move_name(uint8_t move)
{
    /* The names are built the first time they are asked for and then kept,
     * so the returned pointer stays valid. */
    static char names[NUM_MOVES][MOVE_NAME_MAX];

    if (move >= NUM_MOVES) {
        return "?";
    }
    if (names[move][0] == '\0') {
        snprintf(names[move], MOVE_NAME_MAX, "%c%s", FACE_LETTERS[move_face(move)],
                 TURN_SUFFIXES[move_quarter_turns(move) - 1]);
    }
    return names[move];
}

int moves_parse(const char *text, uint8_t *moves, size_t max_moves)
{
    size_t count = 0;
    const char *p = text;

    for (;;) {
        /* Skip blanks; reaching the end of the text means we are done. */
        p += strspn(p, MOVE_SEPARATORS);
        if (*p == '\0') {
            return (int)count;
        }

        /* The first character must be a face letter. Its place in
         * FACE_LETTERS is the face number. */
        const char *letter = strchr(FACE_LETTERS, *p);
        if (letter == NULL || count >= max_moves) {
            return -1;
        }
        p++;

        /* Optional suffix: "2" is a half turn, "'" reverses the direction. */
        uint8_t quarter_turns = 1;
        if (*p == '2') {
            quarter_turns = 2;
            p++;
        }
        if (*p == '\'') {
            quarter_turns = (uint8_t)(QUARTER_TURNS_PER_CYCLE - quarter_turns);
            p++;
        }

        /* Anything else stuck to the move (for example "R3") is an error. */
        if (*p != '\0' && strchr(MOVE_SEPARATORS, *p) == NULL) {
            return -1;
        }
        moves[count++] = move_make((uint8_t)(letter - FACE_LETTERS), quarter_turns);
    }
}

/* ------------------------------------------------------------------ */
/* Validation                                                         */
/* ------------------------------------------------------------------ */

/* Returns true when every value 0..length-1 appears exactly once. */
static bool is_permutation(const uint8_t *perm, uint8_t length)
{
    uint16_t seen = 0;      /* Bit n is set once value n has been seen */
    for (uint8_t i = 0; i < length; i++) {
        if (perm[i] >= length || (seen & (1u << perm[i]))) {
            return false;
        }
        seen |= (uint16_t)(1u << perm[i]);
    }
    return true;
}

/* Checks the laws every reachable cube obeys. A cube that breaks one of
 * them has been taken apart or entered wrongly and cannot be solved. */
CubeStatus cube_validate(const Cube *cube, char *message, size_t message_size)
{
    /* Law 1: every corner piece and every edge piece exists exactly once. */
    if (!is_permutation(cube->cp, NUM_CORNERS)) {
        snprintf(message, message_size, "A corner piece appears more than once");
        return CUBE_ERR_DUPLICATE_CORNER;
    }
    if (!is_permutation(cube->ep, NUM_EDGES)) {
        snprintf(message, message_size, "An edge piece appears more than once");
        return CUBE_ERR_DUPLICATE_EDGE;
    }

    /* Law 2: the corner twists add up to a multiple of 3. */
    unsigned twist_sum = 0;
    for (uint8_t i = 0; i < NUM_CORNERS; i++) {
        twist_sum += cube->co[i];
    }
    if (twist_sum % CORNER_ORIENTATIONS != 0) {
        snprintf(message, message_size, "One corner is twisted in place");
        return CUBE_ERR_TWIST;
    }

    /* Law 3: the edge flips add up to an even number. */
    unsigned flip_sum = 0;
    for (uint8_t i = 0; i < NUM_EDGES; i++) {
        flip_sum += cube->eo[i];
    }
    if (flip_sum % EDGE_ORIENTATIONS != 0) {
        snprintf(message, message_size, "One edge is flipped in place");
        return CUBE_ERR_FLIP;
    }

    /* Law 4: corners and edges have the same permutation parity, because
     * every quarter turn swaps corners and edges the same number of times. */
    if (permutation_parity(cube->cp, NUM_CORNERS) != permutation_parity(cube->ep, NUM_EDGES)) {
        snprintf(message, message_size, "Two pieces are swapped (parity error)");
        return CUBE_ERR_PARITY;
    }

    snprintf(message, message_size, "Cube is valid");
    return CUBE_OK;
}

/* ------------------------------------------------------------------ */
/* Sticker strings                                                    */
/* ------------------------------------------------------------------ */

/* Home face of a sticker index, i.e. the colour it has on a solved cube. */
static uint8_t sticker_face(uint8_t sticker)
{
    return (uint8_t)(sticker / STICKERS_PER_FACE);
}

/* Builds a cube from a 54-character sticker string and validates it.
 * On failure `message` says what is wrong. */
CubeStatus cube_from_stickers(const char *stickers, Cube *cube,
                              char *message, size_t message_size)
{
    uint8_t face_of[NUM_STICKERS];      /* Face each sticker's colour belongs to */
    uint8_t count[NUM_FACES] = { 0 };   /* Stickers seen of each face's colour   */
    char    center[NUM_FACES];          /* Colour character of each face centre  */

    if (strlen(stickers) != NUM_STICKERS) {
        snprintf(message, message_size, "Expected %d stickers, got %u",
                 NUM_STICKERS, (unsigned)strlen(stickers));
        return CUBE_ERR_LENGTH;
    }

    /* The centres are fixed, so they define the colour of each face. */
    for (uint8_t f = 0; f < NUM_FACES; f++) {
        center[f] = stickers[f * STICKERS_PER_FACE + CENTER_STICKER_OFFSET];
        for (uint8_t g = 0; g < f; g++) {
            if (center[g] == center[f]) {
                snprintf(message, message_size, "Centres %c and %c are both colour '%c'",
                         FACE_LETTERS[g], FACE_LETTERS[f], center[f]);
                return CUBE_ERR_CENTERS;
            }
        }
    }

    /* Translate every sticker's colour into the face whose centre has that
     * colour, and count how many stickers each colour has. */
    for (uint8_t i = 0; i < NUM_STICKERS; i++) {
        uint8_t f = 0;
        while (f < NUM_FACES && center[f] != stickers[i]) {
            f++;
        }
        if (f == NUM_FACES) {
            snprintf(message, message_size, "Sticker %u has unknown colour '%c'",
                     (unsigned)i + 1, stickers[i]);
            return CUBE_ERR_UNKNOWN_COLOR;
        }
        face_of[i] = f;
        count[f]++;
    }

    /* Every colour must appear exactly nine times. */
    for (uint8_t f = 0; f < NUM_FACES; f++) {
        if (count[f] != STICKERS_PER_FACE) {
            snprintf(message, message_size, "Colour '%c' appears %u times (expected %d)",
                     center[f], (unsigned)count[f], STICKERS_PER_FACE);
            return CUBE_ERR_COLOR_COUNT;
        }
    }

    /* Identify the corner piece and twist in every corner position by
     * trying each of the 8 pieces in each of its 3 twists until the three
     * colours seen at this position match. */
    for (uint8_t pos = 0; pos < NUM_CORNERS; pos++) {
        bool found = false;
        for (uint8_t piece = 0; piece < NUM_CORNERS && !found; piece++) {
            for (uint8_t twist = 0; twist < CORNER_ORIENTATIONS && !found; twist++) {
                /* Assume a match, then disprove it sticker by sticker. */
                found = true;
                for (uint8_t n = 0; n < STICKERS_PER_CORNER; n++) {
                    uint8_t where = CORNER_STICKERS[pos][(n + twist) % STICKERS_PER_CORNER];
                    if (face_of[where] != sticker_face(CORNER_STICKERS[piece][n])) {
                        found = false;
                    }
                }
                if (found) {
                    cube->cp[pos] = piece;
                    cube->co[pos] = twist;
                }
            }
        }
        if (!found) {
            snprintf(message, message_size, "Corner position %u holds an impossible piece",
                     (unsigned)pos + 1);
            return CUBE_ERR_BAD_CORNER;
        }
    }

    /* Identify the edge piece and flip in every edge position, the same
     * way: 12 pieces, 2 flips each. */
    for (uint8_t pos = 0; pos < NUM_EDGES; pos++) {
        bool found = false;
        for (uint8_t piece = 0; piece < NUM_EDGES && !found; piece++) {
            for (uint8_t flip = 0; flip < EDGE_ORIENTATIONS && !found; flip++) {
                found = true;
                for (uint8_t n = 0; n < STICKERS_PER_EDGE; n++) {
                    uint8_t where = EDGE_STICKERS[pos][(n + flip) % STICKERS_PER_EDGE];
                    if (face_of[where] != sticker_face(EDGE_STICKERS[piece][n])) {
                        found = false;
                    }
                }
                if (found) {
                    cube->ep[pos] = piece;
                    cube->eo[pos] = flip;
                }
            }
        }
        if (!found) {
            snprintf(message, message_size, "Edge position %u holds an impossible piece",
                     (unsigned)pos + 1);
            return CUBE_ERR_BAD_EDGE;
        }
    }

    /* The pieces all exist; now check the twist, flip and parity laws. */
    return cube_validate(cube, message, message_size);
}

/* Writes the cube as a 54-character sticker string, using the face
 * letters U R F D L B as the colours. The reverse of cube_from_stickers. */
void cube_to_stickers(const Cube *cube, char stickers[NUM_STICKERS + 1])
{
    /* Centres never move. */
    for (uint8_t f = 0; f < NUM_FACES; f++) {
        stickers[f * STICKERS_PER_FACE + CENTER_STICKER_OFFSET] = FACE_LETTERS[f];
    }

    /* For each position, paint the piece that sits there. Sticker n of the
     * piece lands on sticker (n + twist) of the position. */
    for (uint8_t pos = 0; pos < NUM_CORNERS; pos++) {
        for (uint8_t n = 0; n < STICKERS_PER_CORNER; n++) {
            uint8_t where = CORNER_STICKERS[pos][(n + cube->co[pos]) % STICKERS_PER_CORNER];
            stickers[where] = FACE_LETTERS[sticker_face(CORNER_STICKERS[cube->cp[pos]][n])];
        }
    }
    for (uint8_t pos = 0; pos < NUM_EDGES; pos++) {
        for (uint8_t n = 0; n < STICKERS_PER_EDGE; n++) {
            uint8_t where = EDGE_STICKERS[pos][(n + cube->eo[pos]) % STICKERS_PER_EDGE];
            stickers[where] = FACE_LETTERS[sticker_face(EDGE_STICKERS[cube->ep[pos]][n])];
        }
    }
    stickers[NUM_STICKERS] = '\0';
}
