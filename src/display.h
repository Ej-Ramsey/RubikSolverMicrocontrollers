/*
 * display.h
 *
 * PLACEHOLDER for the TFT display layer. Nothing is implemented yet; the
 * functions exist so main.c has a place to draw each screen of the app.
 *
 * Sticker maps are arrays of NUM_STICKERS colours (StickerColor values, or
 * COLOR_UNSET for a sticker not entered yet) in the order described in
 * cube.h, so they can be drawn as the unfolded cube shown there.
 */
#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

#include "app.h"
#include "solver.h"

/* Set up the TFT. */
void display_init(void);

/* Editing screen: the unfolded cube with sticker `cursor` highlighted. */
void display_show_entry(const uint8_t stickers[NUM_STICKERS], uint8_t cursor);

/* Error screen: why the entered cube cannot be solved. */
void display_show_message(const char *message);

/* Solution screen: the move list with move `step` (0-based, the next one
 * to make) highlighted, and the cube as it looks after the earlier moves.
 * `step == solution->length` means every move has been made. */
void display_show_solution(const uint8_t stickers[NUM_STICKERS],
                           const Solution *solution, uint8_t step);

/* From the TFTDisplay branch */
void display_show_cube(const Cube *cube);                   /* Draw the unfolded cube    */
void display_show_progress(uint8_t move_number, uint8_t move_count);   /* Solve progress */
void display_stickers_to_map(const char stickers[NUM_STICKERS + 1], uint16_t map[MAP_HEIGHT][MAP_WIDTH]); /* Convert stickers to map of the cube */
void display_map_to_stickers(const uint16_t map[MAP_HEIGHT][MAP_WIDTH], char stickers[NUM_STICKERS + 1]); /* Convert map of the cube to stickers string */


#endif /* DISPLAY_H */
