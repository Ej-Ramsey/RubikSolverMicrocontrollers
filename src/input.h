/*
 * input.h
 *
 * PLACEHOLDER for the IR remote input layer. Nothing is implemented yet.
 *
 * The rest of the program never sees IR codes. The IR driver's only job is
 * to decode a button press and hand back one of the abstract events below;
 * which remote button produces which event is decided entirely in input.c.
 */
#ifndef INPUT_H
#define INPUT_H

/* Everything the user can ask for. The six colour events must stay first
 * and in the same order as StickerColor (app.h), because the app converts
 * between them by offset from INPUT_COLOR_WHITE. */
typedef enum {
    INPUT_NONE,             /* No button pressed since the last poll                    */

    INPUT_COLOR_WHITE,      /* Paint the selected sticker this colour, then move on     */
    INPUT_COLOR_YELLOW,
    INPUT_COLOR_RED,
    INPUT_COLOR_ORANGE,
    INPUT_COLOR_GREEN,
    INPUT_COLOR_BLUE,

    INPUT_NEXT,             /* Editing: next sticker.     Solution: next move           */
    INPUT_PREV,             /* Editing: previous sticker. Solution: previous move       */
    INPUT_NEXT_FACE,        /* Editing: first sticker of the next face                  */
    INPUT_PREV_FACE,        /* Editing: first sticker of the previous face              */
    INPUT_OK,               /* Editing: solve.            Solution / error: back to edit */
    INPUT_BACK,             /* Editing: erase the previous sticker (like backspace)     */
    INPUT_CLEAR             /* Start over with a blank cube                             */
} InputEvent;

void       input_init(void);    /* Set up the IR receiver                              */
InputEvent input_poll(void);    /* Next decoded button press, or INPUT_NONE (never blocks) */

#endif /* INPUT_H */
