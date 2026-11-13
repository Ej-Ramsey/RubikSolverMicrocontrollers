/*
 * motion.h
 *
 * PLACEHOLDER for the movement layer that would turn the solver's move list
 * into physical face turns. Nothing is implemented yet.
 */
#ifndef MOTION_H
#define MOTION_H

#include <stddef.h>
#include <stdint.h>

void motion_init(void);                                         /* Set up the actuators */
void motion_execute_move(uint8_t move);                         /* Perform one move     */
void motion_execute_moves(const uint8_t *moves, size_t count);  /* Perform a move list  */

#endif /* MOTION_H */
