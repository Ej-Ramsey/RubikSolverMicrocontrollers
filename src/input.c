/*
 * input.c
 *
 * PLACEHOLDER for the IR remote input layer. Intentionally empty.
 *
 * When implemented, decode the remote in the background (GPIO interrupt or
 * PIO) and have input_poll() return queued presses, because the main loop
 * only calls it between other work.
 */
#include "input.h"

void input_init(void)
{
}

InputEvent input_poll(void)
{
    return INPUT_NONE;
}
