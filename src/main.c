/*
 * main.c
 *
 * Serial console for the Rubik's Solver. The same file builds for the Pico
 * (USB serial) and for a PC (terminal).
 *
 * Type one of the following and press Enter:
 *   - a scramble, e.g.  R U2 F' L D2 B
 *   - a 54-character sticker string (see cube.h for the order)
 *   - help
 *
 * The program validates the cube, solves it with the lookup tables, checks
 * the answer by applying it, and prints the move list.
 */
#include <stdio.h>
#include <string.h>

#include "cube.h"
#include "display.h"
#include "motion.h"
#include "rubiks_constants.h"
#include "solver.h"

#ifdef PICO_BUILD
#include "pico/stdlib.h"
#else
#include <time.h>
#endif

/* ------------------------------------------------------------------ */
/* Constants                                                          */
/* ------------------------------------------------------------------ */
#define USB_CONNECT_POLL_MS     100     /* Delay between checks for a USB serial connection */
#define MICROSECONDS_PER_MS     1000    /* Unit conversion for timing the solve             */
#define HELP_COMMAND            "help"  /* Command that prints the instructions             */
#define PROMPT                  "\n> "  /* Shown when the console is ready for input        */
#define BACKSPACE_CHAR          '\b'    /* Backspace key as sent by most terminals          */
#define DELETE_CHAR             0x7F    /* Backspace key as sent by other terminals         */

/* ------------------------------------------------------------------ */
/* Platform helpers                                                   */
/* ------------------------------------------------------------------ */

/* On the Pico: start USB serial and wait until a terminal is connected, so
 * the instructions are not printed before anyone can see them.
 * On a PC there is nothing to set up. */
static void platform_init(void)
{
#ifdef PICO_BUILD
    stdio_init_all();
    while (!stdio_usb_connected()) {
        sleep_ms(USB_CONNECT_POLL_MS);
    }
#endif
}

/* Current time in microseconds, for timing the solve. */
static uint64_t time_now_us(void)
{
#ifdef PICO_BUILD
    return time_us_64();
#else
    return (uint64_t)clock() * 1000000u / CLOCKS_PER_SEC;
#endif
}

/* Reads one line into `line`. Returns false when input has ended (PC only). */
static bool read_line(char *line, size_t size)
{
    size_t length = 0;      /* Characters stored in `line` so far */

    for (;;) {
        int c = getchar();

        /* No character: on the Pico just keep waiting; on a PC it means
         * the input was closed (Ctrl+D or end of a piped file). */
        if (c == EOF) {
#ifdef PICO_BUILD
            continue;
#else
            if (length == 0) {
                return false;
            }
            break;
#endif
        }

        /* Enter finishes the line. */
        if (c == '\r' || c == '\n') {
            break;
        }

        /* Backspace removes the last character (and erases it on screen). */
        if (c == BACKSPACE_CHAR || c == DELETE_CHAR) {
            if (length > 0) {
                length--;
#ifdef PICO_BUILD
                printf("\b \b");
#endif
            }
            continue;
        }
        /* Store the character if there is room; extra ones are dropped. */
        if (length < size - 1) {
            line[length++] = (char)c;
#ifdef PICO_BUILD
            putchar(c);     /* Echo, since serial terminals do not */
#endif
        }
    }
#ifdef PICO_BUILD
    printf("\n");
#endif
    line[length] = '\0';
    return true;
}

/* ------------------------------------------------------------------ */
/* Console                                                            */
/* ------------------------------------------------------------------ */

/* Prints the instructions shown at start-up and for the "help" command. */
static void print_help(void)
{
    printf("Rubik's Solver (Thistlethwaite, lookup tables in flash)\n");
    printf("Enter a scramble such as:  R U2 F' L D2 B\n");
    printf("or %d stickers, faces in the order U R F D L B, each face read\n", NUM_STICKERS);
    printf("left to right, top to bottom. Any six letters work as colours;\n");
    printf("the centre stickers define which colour is which face.\n");
}

/* Prints the cube as a sticker string (which can be typed back in). */
static void print_cube(const Cube *cube)
{
    char stickers[NUM_STICKERS + 1];
    cube_to_stickers(cube, stickers);
    printf("Stickers: %s\n", stickers);
}

/* Removes leading and trailing blanks; returns the trimmed string. */
static char *trim(char *text)
{
    while (*text == ' ' || *text == '\t') {
        text++;
    }
    size_t length = strlen(text);
    while (length > 0 && (text[length - 1] == ' ' || text[length - 1] == '\t')) {
        text[--length] = '\0';
    }
    return text;
}

/* Turns one line of input into a cube. Returns false if it is unusable. */
static bool read_cube(const char *text, Cube *cube)
{
    char message[MESSAGE_MAX];

    /* A single word of 54 characters is a sticker string. */
    if (strlen(text) == NUM_STICKERS && strchr(text, ' ') == NULL) {
        if (cube_from_stickers(text, cube, message, sizeof message) != CUBE_OK) {
            printf("Invalid cube: %s\n", message);
            display_show_message(message);
            return false;
        }
        return true;
    }

    /* Otherwise it is a scramble applied to a solved cube. */
    uint8_t scramble[SCRAMBLE_MAX_MOVES];
    int count = moves_parse(text, scramble, SCRAMBLE_MAX_MOVES);
    if (count < 0) {
        printf("Could not read that. Type \"%s\" for instructions.\n", HELP_COMMAND);
        return false;
    }
    cube_reset(cube);
    cube_apply_moves(cube, scramble, (size_t)count);
    return true;
}

/* Solves the cube, prints the answer and hands it to the placeholder
 * display and movement layers. */
static void solve_and_report(const Cube *cube)
{
    Solution solution;

    print_cube(cube);
    display_show_cube(cube);

    /* Solve, timing only the solver itself. */
    uint64_t start = time_now_us();
    SolveStatus status = solver_solve(cube, &solution);
    uint64_t elapsed = time_now_us() - start;

    if (status != SOLVE_OK) {
        printf("Solver error %d\n", (int)status);
        return;
    }

    /* Print the move list, the per-phase counts and the time taken. */
    printf("Solution (%u moves):", (unsigned)solution.length);
    for (uint8_t i = 0; i < solution.length; i++) {
        printf(" %s", move_name(solution.moves[i]));
    }
    printf("\nMoves per phase: %u + %u + %u + %u\n",
           (unsigned)solution.phase_length[0], (unsigned)solution.phase_length[1],
           (unsigned)solution.phase_length[2], (unsigned)solution.phase_length[3]);
    printf("Solve time: %lu.%03lu ms\n",
           (unsigned long)(elapsed / MICROSECONDS_PER_MS),
           (unsigned long)(elapsed % MICROSECONDS_PER_MS));

    /* Check the answer by applying it to a copy of the cube. */
    Cube check = *cube;
    cube_apply_moves(&check, solution.moves, solution.length);
    printf("Check: %s\n", cube_is_solved(&check) ? "solved" : "NOT SOLVED");

    /* Placeholders: these do nothing yet. */
    display_show_progress(solution.length, solution.length);
    motion_execute_moves(solution.moves, solution.length);
}

int main(void)
{
    char line[INPUT_LINE_MAX];

    /* One-time set-up, then the instructions. */
    platform_init();
    display_init();
    motion_init();
    print_help();

    /* Main loop: read a line, turn it into a cube, solve it, repeat. */
    for (;;) {
        printf(PROMPT);
        fflush(stdout);
        if (!read_line(line, sizeof line)) {
            break;      /* Input closed (PC only) */
        }

        /* Ignore empty lines and handle the help command. */
        char *text = trim(line);
        if (*text == '\0') {
            continue;
        }
        if (strcmp(text, HELP_COMMAND) == 0) {
            print_help();
            continue;
        }

        /* Anything else is a scramble or a sticker string. */
        Cube cube;
        if (read_cube(text, &cube)) {
            solve_and_report(&cube);
        }
    }
    return 0;
}
