/*
 * motion.c  (Pico only)
 *
 * Stepper motor control for the six faces. See motion.h for the hardware.
 *
 * A turn is a burst of STEP pulses. Each pulse moves the motor one
 * microstep, so the number of pulses sets how far the face turns and the
 * time between pulses sets how fast. Every turn starts slowly, speeds up,
 * and slows down again at the end (a speed ramp), because a stepper that is
 * asked to start or stop at full speed skips steps and loses its position.
 */
#include "motion.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "cube.h"
#include "rubiks_constants.h"

/* ------------------------------------------------------------------ */
/* Pins (GPIO numbers). Faces are in the order U R F D L B.           */
/* Free of the TFT (GP9-13), the IR receiver (GP15) and the Pico W's  */
/* wireless pins (GP23-25, GP29).                                     */
/* ------------------------------------------------------------------ */
static const uint8_t STEP_PINS[NUM_FACES] = { 2, 3, 4, 5, 6, 7 };       /* Driver STEP input of each face's motor */
static const uint8_t DIR_PINS[NUM_FACES]  = { 16, 17, 18, 19, 20, 21 }; /* Driver DIR input of each face's motor  */
#define ENABLE_PIN              22      /* Shared ENABLE input of all six drivers              */
#define ENABLE_ACTIVE_LEVEL     0       /* ENABLE level that powers the motors (EN is active low on the TMC2209) */

/* ------------------------------------------------------------------ */
/* Directions                                                         */
/* ------------------------------------------------------------------ */
#define CLOCKWISE_DIR_LEVEL     1       /* DIR level that turns a face clockwise (looking at the face) */

/* Set an entry to true if that face turns the wrong way (this depends on
 * how the motor is mounted and how its coils are wired). */
static const bool MOTOR_REVERSED[NUM_FACES] = { false, false, false, false, false, false };

/* ------------------------------------------------------------------ */
/* Steps                                                              */
/* ------------------------------------------------------------------ */
#define FULL_STEPS_PER_REV      200     /* Full steps per motor revolution (1.8 degree motor)          */
#define MICROSTEPS              8       /* TMC2209 microsteps set by MS1/MS2: both GND = 8, both 3.3 V = 16 */
#define QUARTER_TURN_STEPS      (FULL_STEPS_PER_REV * MICROSTEPS / QUARTER_TURNS_PER_CYCLE)  /* 400 */

/* ------------------------------------------------------------------ */
/* Speed (in microsteps per second; 1600 microsteps = one revolution) */
/* Raise MAX_STEP_RATE for faster solves once the turns are reliable; */
/* lower it, or lengthen RAMP_STEPS, if the motors skip.              */
/* ------------------------------------------------------------------ */
#define START_STEP_RATE         400     /* Speed at the start and end of every turn (0.25 rev/s)    */
#define MAX_STEP_RATE           2000    /* Top speed in the middle of a turn (1.25 rev/s)           */
#define RAMP_STEPS              100     /* Microsteps spent speeding up, and again slowing down     */

/* ------------------------------------------------------------------ */
/* Timing                                                             */
/* ------------------------------------------------------------------ */
#define MICROSECONDS_PER_SECOND 1000000u /* Unit conversion for the step delay                       */
#define STEP_PULSE_US           3       /* STEP high time (the TMC2209 needs only about 0.1 us)     */
#define DIR_SETUP_US            5       /* Wait after changing DIR before the first STEP pulse      */
#define ENABLE_SETTLE_MS        10      /* Wait after enabling the drivers before stepping          */
#define MOVE_SETTLE_MS          150     /* Pause after each move so the cube is still before the next */

/* ------------------------------------------------------------------ */
/* State                                                              */
/* ------------------------------------------------------------------ */
static bool drivers_enabled = false;    /* True while the motors are powered */

/* ------------------------------------------------------------------ */
/* Helpers                                                            */
/* ------------------------------------------------------------------ */

/* Speed for step `index` of a turn that is `total` steps long: it climbs
 * from START_STEP_RATE to MAX_STEP_RATE over the first RAMP_STEPS steps and
 * falls back over the last RAMP_STEPS steps. A turn too short to reach top
 * speed simply climbs to its midpoint and comes back down. */
static uint32_t step_rate(uint32_t index, uint32_t total)
{
    /* Distance to the nearer end of the turn. */
    uint32_t from_end = total - 1 - index;
    uint32_t from_edge = (index < from_end) ? index : from_end;

    if (from_edge >= RAMP_STEPS) {
        return MAX_STEP_RATE;
    }
    return START_STEP_RATE + (MAX_STEP_RATE - START_STEP_RATE) * from_edge / RAMP_STEPS;
}

/* Sends `steps` STEP pulses to one motor, following the speed ramp. */
static void pulse_steps(uint8_t face, uint32_t steps)
{
    for (uint32_t i = 0; i < steps; i++) {
        /* Time from one pulse to the next, minus the time spent high. */
        uint32_t period_us = MICROSECONDS_PER_SECOND / step_rate(i, steps);

        gpio_put(STEP_PINS[face], 1);
        busy_wait_us_32(STEP_PULSE_US);
        gpio_put(STEP_PINS[face], 0);
        busy_wait_us_32(period_us - STEP_PULSE_US);
    }
}

/* ------------------------------------------------------------------ */
/* Public functions                                                   */
/* ------------------------------------------------------------------ */

void motion_init(void)
{
    /* Every STEP and DIR pin is an output that starts low. */
    for (uint8_t face = 0; face < NUM_FACES; face++) {
        gpio_init(STEP_PINS[face]);
        gpio_set_dir(STEP_PINS[face], GPIO_OUT);
        gpio_put(STEP_PINS[face], 0);

        gpio_init(DIR_PINS[face]);
        gpio_set_dir(DIR_PINS[face], GPIO_OUT);
        gpio_put(DIR_PINS[face], 0);
    }

    /* Start with the drivers off so the motors stay cool until a solve. */
    gpio_init(ENABLE_PIN);
    gpio_set_dir(ENABLE_PIN, GPIO_OUT);
    drivers_enabled = true;     /* Forces motion_enable() to write the pin */
    motion_enable(false);
}

void motion_enable(bool enabled)
{
    if (enabled == drivers_enabled) {
        return;
    }
    gpio_put(ENABLE_PIN, enabled ? ENABLE_ACTIVE_LEVEL : !ENABLE_ACTIVE_LEVEL);
    drivers_enabled = enabled;

    /* Give the drivers time to energise the coils before the first step. */
    if (enabled) {
        sleep_ms(ENABLE_SETTLE_MS);
    }
}

void motion_turn_face(uint8_t face, int8_t quarter_turns)
{
    if (face >= NUM_FACES || quarter_turns == 0) {
        return;
    }
    motion_enable(true);

    /* Pick the DIR level: clockwise unless the turn is negative, and
     * swapped again for a motor marked as reversed. */
    bool clockwise = quarter_turns > 0;
    if (MOTOR_REVERSED[face]) {
        clockwise = !clockwise;
    }
    gpio_put(DIR_PINS[face], clockwise ? CLOCKWISE_DIR_LEVEL : !CLOCKWISE_DIR_LEVEL);
    busy_wait_us_32(DIR_SETUP_US);

    /* One continuous ramped turn, even for a half turn. */
    uint32_t turns = (uint32_t)(quarter_turns > 0 ? quarter_turns : -quarter_turns);
    pulse_steps(face, turns * QUARTER_TURN_STEPS);

    /* Let the face and the cube stop moving before anything else turns. */
    sleep_ms(MOVE_SETTLE_MS);
}

void motion_execute_move(uint8_t move)
{
    if (move >= NUM_MOVES) {
        return;
    }

    /* R = 1 clockwise quarter turn, R2 = 2, R' = 3, which is quicker to do
     * as 1 counter-clockwise quarter turn. */
    int8_t quarter_turns = (int8_t)move_quarter_turns(move);
    if (quarter_turns == QUARTER_TURNS_PER_CYCLE - 1) {
        quarter_turns = -1;
    }
    motion_turn_face(move_face(move), quarter_turns);
}

void motion_execute_moves(const uint8_t *moves, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        motion_execute_move(moves[i]);
    }
    motion_enable(false);
}
