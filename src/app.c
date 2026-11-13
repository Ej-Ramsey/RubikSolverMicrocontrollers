/*
 * app.c
 *
 * Sticker entry, solving and solution stepping. See app.h.
 */
#include "app.h"

#include <stdio.h>

/* ------------------------------------------------------------------ */
/* Constants                                                          */
/* ------------------------------------------------------------------ */

/* Letter of each colour, indexed by StickerColor. Also used as the colour
 * characters handed to cube_from_stickers(). */
static const char COLOR_LETTERS[NUM_COLORS + 1] = "WYROGB";

/* Shown for a sticker that has not been entered yet. */
#define UNSET_LETTER '-'

/* Face letters in sticker-string order, matching cube_to_stickers(). */
static const char FACE_LETTERS[NUM_FACES + 1] = "URFDLB";

/* Centre colour of each face on a blank cube (standard colour scheme). */
static const uint8_t DEFAULT_CENTERS[NUM_FACES] = {
    COLOR_WHITE,    /* U */
    COLOR_RED,      /* R */
    COLOR_GREEN,    /* F */
    COLOR_YELLOW,   /* D */
    COLOR_ORANGE,   /* L */
    COLOR_BLUE      /* B */
};

/* ------------------------------------------------------------------ */
/* Helpers                                                            */
/* ------------------------------------------------------------------ */

char app_color_letter(uint8_t color)
{
    return color < NUM_COLORS ? COLOR_LETTERS[color] : UNSET_LETTER;
}

/* Index of `letter` in FACE_LETTERS. */
static uint8_t face_from_letter(char letter)
{
    uint8_t f = 0;
    while (f < NUM_FACES && FACE_LETTERS[f] != letter) {
        f++;
    }
    return f;
}

/* Paints `cube` with the colours of the entered centres, so the displayed
 * cube matches the one in the user's hand. */
static void cube_to_colors(const Cube *cube, const uint8_t entered[NUM_STICKERS],
                           uint8_t colors[NUM_STICKERS])
{
    char faces[NUM_STICKERS + 1];
    cube_to_stickers(cube, faces);

    for (uint8_t i = 0; i < NUM_STICKERS; i++) {
        uint8_t f = face_from_letter(faces[i]);
        colors[i] = entered[f * STICKERS_PER_FACE + CENTER_STICKER_OFFSET];
    }
}

/* Cube after the first `app->step` moves of the solution, as colours. */
static void update_shown(App *app)
{
    Cube cube;
    char message[MESSAGE_MAX];

    /* The stickers were accepted when the solution was found, so this
     * conversion cannot fail here. */
    app_stickers_to_cube(app->stickers, &cube, message, sizeof message);
    cube_apply_moves(&cube, app->solution.moves, app->step);
    cube_to_colors(&cube, app->stickers, app->shown);
}

CubeStatus app_stickers_to_cube(const uint8_t stickers[NUM_STICKERS], Cube *cube,
                                char *message, size_t message_size)
{
    char letters[NUM_STICKERS + 1];

    for (uint8_t i = 0; i < NUM_STICKERS; i++) {
        if (stickers[i] >= NUM_COLORS) {
            snprintf(message, message_size, "Sticker %c%u has not been entered",
                     FACE_LETTERS[i / STICKERS_PER_FACE],
                     (unsigned)(i % STICKERS_PER_FACE) + 1);
            return CUBE_ERR_UNKNOWN_COLOR;
        }
        letters[i] = COLOR_LETTERS[stickers[i]];
    }
    letters[NUM_STICKERS] = '\0';

    return cube_from_stickers(letters, cube, message, message_size);
}

/* ------------------------------------------------------------------ */
/* Modes                                                              */
/* ------------------------------------------------------------------ */

void app_clear(App *app)
{
    for (uint8_t i = 0; i < NUM_STICKERS; i++) {
        app->stickers[i] = COLOR_UNSET;
    }
    for (uint8_t f = 0; f < NUM_FACES; f++) {
        app->stickers[f * STICKERS_PER_FACE + CENTER_STICKER_OFFSET] = DEFAULT_CENTERS[f];
    }
    app->cursor = 0;
    app->message[0] = '\0';
    app->solution.length = 0;
    app->step = 0;
    app->mode = APP_EDITING;
}

void app_init(App *app)
{
    app_clear(app);
}

/* Validates the entered stickers and solves them. Ends in APP_SOLVED, or
 * APP_ERROR with the reason in `message`. */
static void solve(App *app)
{
    Cube cube;

    if (app_stickers_to_cube(app->stickers, &cube, app->message, sizeof app->message) != CUBE_OK) {
        app->mode = APP_ERROR;
        return;
    }
    if (solver_solve(&cube, &app->solution) != SOLVE_OK) {
        snprintf(app->message, sizeof app->message, "Solver failed (lookup tables corrupt?)");
        app->mode = APP_ERROR;
        return;
    }

    app->step = 0;
    update_shown(app);
    app->mode = APP_SOLVED;
}

/* Editing: paint, move the cursor, erase, or solve. */
static bool handle_editing(App *app, InputEvent event)
{
    if (event >= INPUT_COLOR_WHITE && event < INPUT_COLOR_WHITE + NUM_COLORS) {
        app->stickers[app->cursor] = (uint8_t)(event - INPUT_COLOR_WHITE);
        /* Move on to the next sticker, staying on the last one at the end. */
        if (app->cursor < NUM_STICKERS - 1) {
            app->cursor++;
        }
        return true;
    }

    switch (event) {
    case INPUT_NEXT:
        app->cursor = (uint8_t)((app->cursor + 1) % NUM_STICKERS);
        return true;
    case INPUT_PREV:
        app->cursor = (uint8_t)((app->cursor + NUM_STICKERS - 1) % NUM_STICKERS);
        return true;
    case INPUT_NEXT_FACE:
        app->cursor = (uint8_t)((app->cursor / STICKERS_PER_FACE + 1) % NUM_FACES * STICKERS_PER_FACE);
        return true;
    case INPUT_PREV_FACE:
        app->cursor = (uint8_t)((app->cursor / STICKERS_PER_FACE + NUM_FACES - 1) % NUM_FACES *
                                STICKERS_PER_FACE);
        return true;
    case INPUT_BACK:
        /* Like backspace: step back over the sticker just painted and
         * erase it. On the first sticker, just erase it. */
        if (app->cursor > 0) {
            app->cursor--;
        }
        app->stickers[app->cursor] = COLOR_UNSET;
        return true;
    case INPUT_CLEAR:
        app_clear(app);
        return true;
    case INPUT_OK:
        solve(app);
        return true;
    default:
        return false;
    }
}

/* Showing the solution: step through the moves or go back to editing. */
static bool handle_solved(App *app, InputEvent event)
{
    switch (event) {
    case INPUT_NEXT:
        if (app->step >= app->solution.length) {
            return false;
        }
        app->step++;
        update_shown(app);
        return true;
    case INPUT_PREV:
        if (app->step == 0) {
            return false;
        }
        app->step--;
        update_shown(app);
        return true;
    case INPUT_OK:
    case INPUT_BACK:
        app->mode = APP_EDITING;    /* Keep the stickers so they can be corrected */
        return true;
    case INPUT_CLEAR:
        app_clear(app);
        return true;
    default:
        return false;
    }
}

bool app_handle_input(App *app, InputEvent event)
{
    if (event == INPUT_NONE) {
        return false;
    }

    switch (app->mode) {
    case APP_EDITING:
        return handle_editing(app, event);
    case APP_SOLVED:
        return handle_solved(app, event);
    case APP_ERROR:
    default:
        /* Any button dismisses the error; CLEAR also starts over. */
        if (event == INPUT_CLEAR) {
            app_clear(app);
        } else {
            app->mode = APP_EDITING;
        }
        return true;
    }
}
