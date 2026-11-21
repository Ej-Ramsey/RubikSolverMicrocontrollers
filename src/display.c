/*
 * display.c  (Pico only)
 *
 * TFT display layer on the ST7735 (pico-st7735 library, SPI1). The top of
 * the screen shows the unfolded cube from cube.h; the bottom holds one or
 * two lines of text for the current screen.
 */
#include "display.h"

#include <stdbool.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "ST7735_TFT.h"
#include "hw.h"

/* Screen colour of each StickerColor (same order as app.h). */
static const uint16_t COLOR_RGB565[NUM_COLORS] = {
    ST7735_WHITE,   /* COLOR_WHITE  */
    ST7735_YELLOW,  /* COLOR_YELLOW */
    ST7735_RED,     /* COLOR_RED    */
    ST7735_ORANGE,  /* COLOR_ORANGE */
    ST7735_GREEN,   /* COLOR_GREEN  */
    ST7735_BLUE     /* COLOR_BLUE   */
};

/* Top-left square of each face on the map, in sticker-string order
 * (U R F D L B), as {row, column}. Matches the layout drawn in cube.h. */
static const uint8_t FACE_ORIGIN[NUM_FACES][2] = {
    {0, 3},     /* U */
    {3, 6},     /* R */
    {3, 3},     /* F */
    {6, 3},     /* D */
    {3, 0},     /* L */
    {3, 9}      /* B */
};

static const char FACE_LETTERS[NUM_FACES + 1] = "URFDLB";

/* Sticker the cursor outline was last drawn around. */
static uint8_t last_cursor = NUM_STICKERS;

/* Map row and column of sticker `index`. */
static void sticker_cell(uint8_t index, uint8_t *row, uint8_t *col)
{
    uint8_t face = index / STICKERS_PER_FACE;
    uint8_t i = index % STICKERS_PER_FACE;
    *row = FACE_ORIGIN[face][0] + i / FACE_LENGTH;
    *col = FACE_ORIGIN[face][1] + i % FACE_LENGTH;
}

static uint8_t cell_x(uint8_t col) { return (uint8_t)(col * (STICKER_SIZE + 1) + MAP_OFFSET_X); }
static uint8_t cell_y(uint8_t row) { return (uint8_t)(row * (STICKER_SIZE + 1) + MAP_OFFSET_Y); }

/* Fills the map with the screen colour of each sticker; 0 = no sticker. */
static void colors_to_map(const uint8_t stickers[NUM_STICKERS], uint16_t map[MAP_HEIGHT][MAP_WIDTH])
{
    for (uint8_t y = 0; y < MAP_HEIGHT; y++) {
        for (uint8_t x = 0; x < MAP_WIDTH; x++) {
            map[y][x] = 0;
        }
    }
    for (uint8_t i = 0; i < NUM_STICKERS; i++) {
        uint8_t row, col;
        sticker_cell(i, &row, &col);
        map[row][col] = stickers[i] < NUM_COLORS ? COLOR_RGB565[stickers[i]] : COLOR_UNSET_RGB565;
    }
}

/* Draws the unfolded cube. */
static void draw_cube(const uint8_t stickers[NUM_STICKERS])
{
    uint16_t map[MAP_HEIGHT][MAP_WIDTH];
    colors_to_map(stickers, map);

    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            if (map[y][x] != 0) {
                fillRect(cell_x(x), cell_y(y), STICKER_SIZE, STICKER_SIZE, map[y][x]);
            }
        }
    }
}

/* Draws (or erases, with black) the outline around sticker `index`. */
static void draw_outline(uint8_t index, uint16_t color)
{
    uint8_t row, col;
    sticker_cell(index, &row, &col);
    drawRectWH(cell_x(col) - 1, cell_y(row) - 1, STICKER_SIZE + 2, STICKER_SIZE + 2, color);
}

static void erase_outline(void)
{
    if (last_cursor < NUM_STICKERS) {
        draw_outline(last_cursor, ST7735_BLACK);
        last_cursor = NUM_STICKERS;
    }
}

/* Blanks the text area under the cube. */
static void clear_text(void)
{
    fillRect(0, TEXT_TOP, tft_width, tft_height - TEXT_TOP, ST7735_BLACK);
}

void display_init(void)
{
    spi_init(SPI_PORT, 1000000);
    gpio_set_function(SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SPI_TX, GPIO_FUNC_SPI);
    tft_spi_init();
    TFT_BlackTab_Initialize();
    setTextWrap(true);
    fillScreen(ST7735_BLACK);
}

void display_show_entry(const uint8_t stickers[NUM_STICKERS], uint8_t cursor)
{
    char line[24];

    erase_outline();
    draw_cube(stickers);
    draw_outline(cursor, ST7735_MAGENTA);
    last_cursor = cursor;

    clear_text();
    snprintf(line, sizeof line, "Sticker %c%u",
             FACE_LETTERS[cursor / STICKERS_PER_FACE], (unsigned)(cursor % STICKERS_PER_FACE) + 1);
    drawText(4, TEXT_TOP + 4, line, ST7735_WHITE, ST7735_BLACK, 1);
    drawText(4, TEXT_TOP + 20, "1-6 paint  OK solve", ST7735_CYAN, ST7735_BLACK, 1);
    drawText(4, TEXT_TOP + 32, "0 erase    7 clear", ST7735_CYAN, ST7735_BLACK, 1);
}

void display_show_message(const char *message)
{
    /* The entered cube stays on screen so the bad sticker can be found. */
    erase_outline();
    clear_text();
    drawText(4, TEXT_TOP + 4, "Can't solve:", ST7735_RED, ST7735_BLACK, 1);
    drawText(4, TEXT_TOP + 18, message, ST7735_WHITE, ST7735_BLACK, 1);
}

void display_show_solution(const uint8_t stickers[NUM_STICKERS],
                           const Solution *solution, uint8_t step)
{
    char line[24];

    erase_outline();
    draw_cube(stickers);
    clear_text();

    if (step < solution->length) {
        snprintf(line, sizeof line, "Move %u of %u", (unsigned)step + 1, (unsigned)solution->length);
        drawText(4, TEXT_TOP + 4, line, ST7735_WHITE, ST7735_BLACK, 1);
        drawText(4, TEXT_TOP + 18, move_name(solution->moves[step]), ST7735_YELLOW, ST7735_BLACK, 2);
    } else {
        snprintf(line, sizeof line, "Solved in %u moves", (unsigned)solution->length);
        drawText(4, TEXT_TOP + 4, line, ST7735_GREEN, ST7735_BLACK, 1);
    }
    drawText(4, TEXT_TOP + 40, "<< >> step  OK edit", ST7735_CYAN, ST7735_BLACK, 1);
}
