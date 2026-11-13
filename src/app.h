/*
 * app.h
 *
 * The Rubik's Solver application, independent of any hardware: it holds the
 * sticker map being entered, reacts to input events, runs the solver and
 * keeps track of which solution move is being shown.
 *
 * main.c feeds it events from the IR remote (input.h) and redraws the TFT
 * (display.h) from its state. Because nothing here touches the hardware, the
 * same code is exercised by the PC tests.
 *
 * Modes:
 *   APP_EDITING  The user paints the 54 stickers one at a time. A cursor
 *                marks the selected sticker; painting moves it forward.
 *   APP_ERROR    The entered cube is impossible. `message` says why. Any
 *                button returns to editing with the stickers kept.
 *   APP_SOLVED   `solution` holds the answer. NEXT / PREV step through it,
 *                and `shown` is the cube after the first `step` moves.
 */
#ifndef APP_H
#define APP_H

#include <stdbool.h>
#include <stdint.h>

#include "cube.h"
#include "input.h"
#include "rubiks_constants.h"
#include "solver.h"

/* Physical sticker colours, in the same order as the INPUT_COLOR_* events. */
typedef enum {
    COLOR_WHITE,
    COLOR_YELLOW,
    COLOR_RED,
    COLOR_ORANGE,
    COLOR_GREEN,
    COLOR_BLUE
} StickerColor;

typedef enum {
    APP_EDITING,
    APP_ERROR,
    APP_SOLVED
} AppMode;

typedef struct {
    AppMode mode;
    uint8_t stickers[NUM_STICKERS];     /* Entered colours (StickerColor or COLOR_UNSET)  */
    uint8_t cursor;                     /* Selected sticker while editing (0..53)        */
    char    message[MESSAGE_MAX];       /* Reason the cube was rejected (APP_ERROR)       */
    Solution solution;                  /* Answer found by the solver (APP_SOLVED)        */
    uint8_t step;                       /* Solution moves already shown (0..length)       */
    uint8_t shown[NUM_STICKERS];        /* Colours of the cube after `step` moves         */
} App;

/* Starts in editing mode with a blank cube (see app_clear). */
void app_init(App *app);

/* Blank cube: every sticker unset except the centres, which get the usual
 * colour scheme (U white, R red, F green, D yellow, L orange, B blue). The
 * centres can be repainted if the cube is held differently. */
void app_clear(App *app);

/* Applies one input event. Returns true if anything the display shows has
 * changed and the screen should be redrawn. */
bool app_handle_input(App *app, InputEvent event);

/* Converts a colour sticker map into a cube. Returns CUBE_OK, or an error
 * with a readable explanation in `message` (including unentered stickers). */
CubeStatus app_stickers_to_cube(const uint8_t stickers[NUM_STICKERS], Cube *cube,
                                char *message, size_t message_size);

/* Single-letter name of a colour ("W", "Y", ...), or '-' when unset. */
char app_color_letter(uint8_t color);

#endif /* APP_H */
