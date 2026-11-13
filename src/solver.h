/*
 * solver.h
 *
 * Thistlethwaite solver. Each of the four phases measures a coordinate of
 * the cube, looks up its distance to the phase goal in a table stored in
 * flash, and repeatedly takes any allowed move that lowers the distance.
 *
 * The solver only produces a plain move list; it knows nothing about the
 * display or the servos.
 */
#ifndef SOLVER_H
#define SOLVER_H

#include <stdint.h>

#include "cube.h"

typedef enum {
    SOLVE_OK,               /* A solution was found                              */
    SOLVE_ERR_INVALID_CUBE, /* The cube state failed validation                  */
    SOLVE_ERR_TABLE         /* The lookup tables do not match the cube (corrupt) */
} SolveStatus;

typedef struct {
    uint8_t moves[MAX_SOLUTION_MOVES];  /* Final move list                              */
    uint8_t length;                     /* Number of moves in `moves`                   */
    uint8_t phase_length[NUM_PHASES];   /* Moves found by each phase (before merging)   */
} Solution;

/* Solves `cube` (which is not modified) and fills in `solution`. */
SolveStatus solver_solve(const Cube *cube, Solution *solution);

/* Distance (in phase moves) from `cube` to the goal of `phase` (0..3). */
uint8_t solver_phase_distance(const Cube *cube, uint8_t phase);

#endif /* SOLVER_H */
