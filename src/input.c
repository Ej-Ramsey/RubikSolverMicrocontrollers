/*
 * input.c  (Pico only)
 *
 * IR remote input. The receiver output is decoded as NEC in a GPIO
 * interrupt (ported from the Oct 29 prototype's ir_nec.c), so presses are
 * queued while the main loop is busy drawing or solving. input_poll() turns
 * each queued command byte into an InputEvent with IR_MAP below.
 *
 * NEC timing between falling edges (receiver output is low during a burst):
 *   leader 9 ms + 4.5 ms = 13.5 ms, bit 0 = 1.125 ms, bit 1 = 2.25 ms.
 * 32 bits, LSB first: address, ~address, command, ~command. Repeat codes
 * (held button) are ignored.
 */
#include "input.h"

#include <stddef.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#ifdef RUBIKS_DEBUG_LOG
#include <stdio.h>
#endif

#ifndef IR_PIN
#define IR_PIN 15           /* IR receiver output (same pin as the IR labs) */
#endif

#define IR_QUEUE_LEN 16

/* Remote command byte -> event. These codes are for the common 21-key kit
 * remote (Elegoo / "Car MP3"); with RUBIKS_DEBUG_LOG on, unknown codes are
 * printed so the table can be fixed for a different remote. Keys 1-6 follow
 * the face order U R F D L B with the usual colour scheme. */
typedef struct {
    uint8_t    code;
    InputEvent event;
} IrMapping;

static const IrMapping IR_MAP[] = {
    {0x0C, INPUT_COLOR_WHITE},  /* 1                */
    {0x18, INPUT_COLOR_RED},    /* 2                */
    {0x5E, INPUT_COLOR_GREEN},  /* 3                */
    {0x08, INPUT_COLOR_YELLOW}, /* 4                */
    {0x1C, INPUT_COLOR_ORANGE}, /* 5                */
    {0x5A, INPUT_COLOR_BLUE},   /* 6                */
    {0x44, INPUT_PREV},         /* |<<              */
    {0x43, INPUT_NEXT},         /* >>|              */
    {0x09, INPUT_PREV_FACE},    /* up / EQ          */
    {0x07, INPUT_NEXT_FACE},    /* down / VOL-      */
    {0x40, INPUT_OK},           /* play / pause     */
    {0x16, INPUT_BACK},         /* 0                */
    {0x42, INPUT_CLEAR},        /* 7                */
};

/* Filled by the interrupt, emptied by input_poll(). */
static volatile uint8_t queue[IR_QUEUE_LEN];
static volatile uint8_t q_head, q_tail;

static uint32_t last_edge, bits;
static int nbits = -1;      /* -1 = waiting for a leader */

static void on_ir_edge(uint gpio, uint32_t events)
{
    (void)gpio;
    (void)events;
    uint32_t now = time_us_32();
    uint32_t dt = now - last_edge;
    last_edge = now;

    if (dt > 12500 && dt < 14500) {             /* leader */
        nbits = 0;
        bits = 0;
        return;
    }
    if (nbits < 0) {
        return;
    }
    if (dt > 800 && dt < 1500) {                /* 0 bit */
        nbits++;
    } else if (dt > 1800 && dt < 2700) {        /* 1 bit */
        bits |= 1u << nbits;
        nbits++;
    } else {                                    /* noise or repeat code */
        nbits = -1;
        return;
    }

    if (nbits == 32) {
        nbits = -1;
        uint8_t cmd = (uint8_t)(bits >> 16);
        uint8_t inv = (uint8_t)(bits >> 24);
        if ((uint8_t)(cmd ^ inv) == 0xFF) {
            uint8_t next = (uint8_t)((q_head + 1) % IR_QUEUE_LEN);
            if (next != q_tail) {               /* drop the press if full */
                queue[q_head] = cmd;
                q_head = next;
            }
        }
    }
}

void input_init(void)
{
    gpio_init(IR_PIN);
    gpio_set_dir(IR_PIN, GPIO_IN);
    gpio_pull_up(IR_PIN);
    gpio_set_irq_enabled_with_callback(IR_PIN, GPIO_IRQ_EDGE_FALL, true, on_ir_edge);
}

InputEvent input_poll(void)
{
    while (q_tail != q_head) {
        uint8_t cmd = queue[q_tail];
        q_tail = (uint8_t)((q_tail + 1) % IR_QUEUE_LEN);

        for (size_t i = 0; i < sizeof IR_MAP / sizeof IR_MAP[0]; i++) {
            if (IR_MAP[i].code == cmd) {
                return IR_MAP[i].event;
            }
        }
#ifdef RUBIKS_DEBUG_LOG
        printf("IR: unknown code 0x%02X\n", (unsigned)cmd);
#endif
    }
    return INPUT_NONE;
}
