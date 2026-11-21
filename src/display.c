/*
 * display.c
 *
 * PLACEHOLDER for the TFT display layer. Intentionally empty.
 */
#include "display.h"

#ifdef PICO_BUILD
#include "ST7735_TFT.h"
#include "hw.h"
#include "hardware/spi.h"
#include "hardware/timer.h"
#include <stdbool.h>
#include "pico/stdlib.h"
#include <stdio.h>
#else
#include <time.h>
#endif

void display_init(void)
{
    spi_init(SPI_PORT, 1000000);
    gpio_set_function(SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SPI_TX, GPIO_FUNC_SPI);
    tft_spi_init();
    TFT_BlackTab_Initialize();
    setTextWrap(true);
    fillScreen(ST7735_BLACK);
    drawText(10, 20, "Hello World", ST7735_WHITE, ST7735_BLACK, 1);
    drawText(10, 40, "RG GB", ST7735_WHITE, ST7735_BLACK, 1);

    Cube initCube;
    cube_reset(&initCube);
    display_show_cube(&initCube);
}

void display_show_cube(const Cube *cube)
{
    char stickers[NUM_STICKERS + 1];
    uint16_t map[MAP_HEIGHT][MAP_WIDTH];
    cube_to_stickers(cube, stickers);
    display_stickers_to_map(stickers, map);

    // Draw each sticker on the map
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            if (map[y][x] != 0) {
                // 128 x 160 display
                fillRect(x * (STICKER_SIZE + 1) + MAP_OFFSET_X,
                         y * (STICKER_SIZE + 1) + MAP_OFFSET_Y,
                         STICKER_SIZE, STICKER_SIZE, map[y][x]);
            }
        }
    }
}

void display_stickers_to_map(const char stickers[NUM_STICKERS + 1], uint16_t map[MAP_HEIGHT][MAP_WIDTH])
{
    // Blank map to make filling of the array easier
    const uint8_t blankMap[MAP_HEIGHT][MAP_WIDTH] = {
    {0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},   
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},   
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},   
    {0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0}
    };

    uint8_t stickerIndex = 0;
    // Outer two loops go through blocks of nine (or 1 face at a time) on the map
    for (int8_t facex = 0; facex < MAP_WIDTH / 3; facex++) {
        for (int8_t facey = 0; facey < MAP_HEIGHT / 3; facey++) {

            // Inner two loops go through all 9 squares of each face
            for (int8_t x = 0; x < FACE_LENGTH; x++) {
                for (int8_t y = 0; y < FACE_LENGTH; y++) {
                    uint16_t *location = &map[(facey * 3) + y][(facex * 3) + x];
                    if (blankMap[(facey * 3) + y][(facex * 3) + x] == 0) {
                        *location = 0;
                    }
                    else if (blankMap[(facey * 3) + y][(facex * 3) + x] == 1) {
                        if (stickers[stickerIndex] == 'U') {
                            *location = ST7735_WHITE;
                        }
                        else if (stickers[stickerIndex] == 'R') {
                            *location = ST7735_RED;
                        }
                        else if (stickers[stickerIndex] == 'F') {
                            *location = ST7735_GREEN;
                        }
                        else if (stickers[stickerIndex] == 'D') {
                            *location = ST7735_YELLOW;
                        }
                        else if (stickers[stickerIndex] == 'L') {
                            *location = ST7735_ORANGE;
                        }
                        else if (stickers[stickerIndex] == 'B') {
                            *location = ST7735_BLUE;
                        }
                        stickerIndex++;
                    }
                }
            }

        }
    }
}

void display_map_to_stickers(const uint16_t map[MAP_HEIGHT][MAP_WIDTH], char stickers[NUM_STICKERS + 1])
{
    uint8_t stickerIndex = 0;
    // Outer two loops go through blocks of nine (or 1 face at a time) on the map
    for (int8_t facex = 0; facex < MAP_WIDTH / 3; facex++) {
        for (int8_t facey = 0; facey < MAP_HEIGHT / 3; facey++) {

            // Inner two loops go through all 9 squares of each face
            for (int8_t x = 0; x < FACE_LENGTH; x++) {
                for (int8_t y = 0; y < FACE_LENGTH; y++) {
                    const uint16_t *location = &map[(facey * 3) + y][(facex * 3) + x];
                    if (*location != 0) {
                        if (*location == ST7735_WHITE) {
                            stickers[stickerIndex] = 'U';
                        }
                        else if (*location == ST7735_RED) {
                            stickers[stickerIndex] = 'R';
                        }
                        else if (*location == ST7735_GREEN) {
                            stickers[stickerIndex] = 'F';
                        }
                        else if (*location == ST7735_YELLOW) {
                            stickers[stickerIndex] = 'D';
                        }
                        else if (*location == ST7735_ORANGE) {
                            stickers[stickerIndex] = 'L';
                        }
                        else if (*location == ST7735_BLUE) {
                            stickers[stickerIndex] = 'B';
                        }
                        stickerIndex++;
                    }
                }
            }

        }
    }
    stickers[NUM_STICKERS] = '\0';
}

void display_show_message(const char *message)
{
    (void)message;
}

void display_show_progress(uint8_t move_number, uint8_t move_count)
{
    (void)move_number;
    (void)move_count;
}

void display_show_entry(const uint8_t stickers[NUM_STICKERS], uint8_t cursor)
{
    (void)stickers;
    (void)cursor;
}

void display_show_solution(const uint8_t stickers[NUM_STICKERS],
                           const Solution *solution, uint8_t step)
{
    (void)stickers;
    (void)solution;
    (void)step;
}
