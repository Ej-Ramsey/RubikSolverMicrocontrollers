/*
 * display.c
 *
 * PLACEHOLDER for the TFT display layer. Intentionally empty.
 */
#include "display.h"

void display_init(void)
{
}

void display_show_entry(const uint8_t stickers[NUM_STICKERS], uint8_t cursor)
{
    (void)stickers;
    (void)cursor;
}

void display_show_message(const char *message)
{
    (void)message;
}

void display_show_solution(const uint8_t stickers[NUM_STICKERS],
                           const Solution *solution, uint8_t step)
{
    (void)stickers;
    (void)solution;
    (void)step;
}
