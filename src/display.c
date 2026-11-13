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
}

void display_show_cube(const Cube *cube)
{
    (void)cube;
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
