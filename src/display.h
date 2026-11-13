/*
 * display.h
 *
 * PLACEHOLDER for the TFT display layer. Nothing is implemented yet; the
 * functions exist only so the rest of the program has a place to call.
 */
#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

#include "cube.h"

void display_init(void);                                    /* Set up the TFT            */
void display_show_cube(const Cube *cube);                   /* Draw the unfolded cube    */
void display_show_message(const char *message);             /* Show status / error text  */
void display_show_progress(uint8_t move_number, uint8_t move_count);   /* Solve progress */

#endif /* DISPLAY_H */
