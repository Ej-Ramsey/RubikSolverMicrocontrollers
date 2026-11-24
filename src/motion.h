/*
 * motion.h  (Pico only)
 *
 * Movement layer: turns the solver's move list into physical face turns.
 *
 * Hardware: six NEMA 17 stepper motors, one per face, each driven by a
 * TMC2209 stepper driver (BIGTREETECH V1.3) in STEP/DIR mode, powered from
 * the 12 V supply. The UART pin is not used. Every driver gets its own STEP
 * and DIR pin; all six share one ENABLE pin. Motor n turns face n, in the face order U R F D L B.
 *
 * Pins, microstepping, speeds and directions are constants at the top of
 * motion.c.
 */
#ifndef MOTION_H
#define MOTION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Sets up the STEP, DIR and ENABLE pins. The drivers start disabled. */
void motion_init(void);

/* Powers the motors (true) or lets them go limp (false). While enabled the
 * motors hold their faces still, but the motors and drivers warm up. */
void motion_enable(bool enabled);

/* Turns one face by a signed number of quarter turns: positive is clockwise
 * looking at that face, negative is counter-clockwise. For calibration and
 * testing; the solver's moves go through motion_execute_move(). */
void motion_turn_face(uint8_t face, int8_t quarter_turns);

/* Performs one solver move (for example R, U2 or F'). Enables the drivers
 * if needed and waits until the face has stopped before returning. */
void motion_execute_move(uint8_t move);

/* Performs a whole move list, then disables the drivers. */
void motion_execute_moves(const uint8_t *moves, size_t count);

#endif /* MOTION_H */
