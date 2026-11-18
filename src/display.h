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
void display_stickers_to_map(const char stickers[NUM_STICKERS + 1], uint16_t map[MAP_HEIGHT][MAP_WIDTH]); /* Convert stickers to map of the cube */
void display_map_to_stickers(const uint16_t map[MAP_HEIGHT][MAP_WIDTH], char stickers[NUM_STICKERS + 1]); /* Convert map of the cube to stickers string */


#endif /* DISPLAY_H */
