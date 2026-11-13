/*
 * main.c  (Pico only)
 *
 * Runs the Rubik's Solver entirely on the Pico. No PC or cable is needed:
 *   - the sticker map is entered with the IR remote   (input.c,   placeholder)
 *   - every screen is drawn on the TFT                 (display.c, placeholder)
 *   - the solve uses the lookup tables in flash        (solver.c)
 *
 * The loop is: poll the remote -> let the app react -> redraw if needed.
 * All of the behaviour lives in app.c so it can be tested on a PC.
 *
 * Optional debugging: configure with -DRUBIKS_DEBUG_USB=ON to print each
 * state change over USB serial. This is output only; the Pico never waits
 * for or reads from a computer.
 */
#include "pico/stdlib.h"

#include "app.h"
#include "display.h"
#include "input.h"
#include "motion.h"

#ifdef RUBIKS_DEBUG_LOG
#include <stdio.h>
#define DEBUG_LOG(...) printf(__VA_ARGS__)
#else
#define DEBUG_LOG(...) ((void)0)
#endif

/* The app lives here rather than on the stack. */
static App app;

/* Draws the screen that matches the app's current mode. */
static void render(const App *state)
{
    switch (state->mode) {
    case APP_EDITING:
        display_show_entry(state->stickers, state->cursor);
        break;
    case APP_ERROR:
        display_show_message(state->message);
        break;
    case APP_SOLVED:
        display_show_solution(state->shown, &state->solution, state->step);
        break;
    }
}

#ifdef RUBIKS_DEBUG_LOG
/* Prints the app state over USB serial (debug builds only). */
static void debug_report(const App *state)
{
    switch (state->mode) {
    case APP_EDITING:
        printf("Edit [");
        for (uint8_t i = 0; i < NUM_STICKERS; i++) {
            putchar(app_color_letter(state->stickers[i]));
        }
        printf("] cursor %u\n", (unsigned)state->cursor);
        break;
    case APP_ERROR:
        printf("Error: %s\n", state->message);
        break;
    case APP_SOLVED:
        printf("Solution (%u moves), step %u:", (unsigned)state->solution.length,
               (unsigned)state->step);
        for (uint8_t i = 0; i < state->solution.length; i++) {
            printf(" %s", move_name(state->solution.moves[i]));
        }
        printf("\n");
        break;
    }
}
#endif

int main(void)
{
#ifdef RUBIKS_DEBUG_LOG
    stdio_init_all();
#endif
    display_init();
    input_init();
    motion_init();

    app_init(&app);
    render(&app);

    for (;;) {
        InputEvent event = input_poll();
        if (event == INPUT_NONE) {
            tight_loop_contents();
            continue;
        }

        AppMode before = app.mode;
        uint64_t start = time_us_64();
        bool changed = app_handle_input(&app, event);
        uint64_t elapsed = time_us_64() - start;
        (void)elapsed;

        if (!changed) {
            continue;
        }
        render(&app);

#ifdef RUBIKS_DEBUG_LOG
        debug_report(&app);
#endif
        /* A new solution was just found. */
        if (before != APP_SOLVED && app.mode == APP_SOLVED) {
            DEBUG_LOG("Solve took %lu us\n", (unsigned long)elapsed);
            motion_execute_moves(app.solution.moves, app.solution.length);  /* Placeholder */
        }
    }
}
