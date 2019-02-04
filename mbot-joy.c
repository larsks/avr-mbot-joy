/**
 * \file mbot-joy.c
 *
 * Control your mbot with a joystick.
 *
 * Connect a standard 5-pin arcade joystick to your mbot (pins GND, 9, 10,
 * 11, 12) and control its movements. This code makes use of the ultrasonic
 * sensor to avoid bumping into obstacles.
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

//! \defgroup OBSTACLE Obstacle avoidance settings
//! @{
#define MAX_SPEED  200      //!< Normal forward speed
#define MED_SPEED  127      //!< Slow speed when approaching an obstacle
#define STOP_DISTANCE 150   //!< Stop when <= this distance from an obstacle
#define SLOW_DISTANCE 500   //!< Slow down when <= this distance from an obstacle
//! @}


//! \defgroup PINS Pins and ports
//! @{
#define LEDDDR     (DDRB)           //!< DDR register for LED pin
#define LEDPORTREG (PORTB)          //!< PORT register for LED pin
#define LEDPINREG  (PINB)           //!< PIN register for LED pin
#define LEDPIN     (_BV(PORTB5))    //!< LED pin number

#define USDDR     (DDRC)            //!< DDR register for ultrasonic sensor
#define USPORTREG (PORTC)           //!< PORT register for ultrasonic sensor
#define USPINREG  (PINC)            //!< PIN register for ultrasonic sensor
#define USPIN     (_BV(PORTC1))     //!< Data pin for ultrasonic sensor

#define MOTORDDR  (DDRD)            //!< DDR register for motors
#define MOTORPORT (PORTD)           //!< PORT register for motors
#define MLEFTDIR  (_BV(PORTD4))     //!< Left motor direction pin
#define MLEFTPWM  (_BV(PORTD5))     //!< Left motor speed control
#define MRIGHTDIR (_BV(PORTD7))     //!< Right motor direction pin
#define MRIGHTPWM (_BV(PORTD6))     //!< Right motor speed control
#define MLEFTOCR  (OCR0A)           //!< Left motor OCR register
#define MRIGHTOCR (OCR0B)           //!< Right motor OCR register

#define JOYDDR    (DDRB)            //!< DDR register for joystick
#define JOYPORT   (PORTB)           //!< PORT register for joystick
#define JOYPIN    (PINB)            //!< PIN register for joystick
#define PIN_LEFT  (_BV(PORTB1))     //!< Joystick left pin
#define PIN_RIGHT (_BV(PORTB2))     //!< Joystick right pin
#define PIN_DOWN  (_BV(PORTB3))     //!< Joystick down (reverse) pin
#define PIN_UP    (_BV(PORTB4))     //!< Joystick up (forward) pin
//! @}

//! Describe motor direction
typedef enum DIRECTION {STOPPED, FORWARD, REVERSE} DIRECTION;

//! Describe current turning state
typedef enum TURN {NONE, LEFT, RIGHT} TURN;

//! States in ultrasonic measurement state machine
typedef enum DISTANCE_STATE {
    PING,
    WAIT_PULSE_START,
    WAIT_PULSE_END,
    INTERVAL
} DISTANCE_STATE;

/**
 * Configure pins and pwm.
 *
 * Set up fast PWM mode with OCR output enabled on for the motor speed
 * control pins, OC0A (PORTD6) and OC0B (PORTD5). The motor PWM frequency
 * is 967Hz.  At 16Mhz with a prescaler of clk/64, this gets us
 * 976.5625Hz, which is apparenlty "close enough" (that's
 * `16000000 / 64 / 255`, where the final `/255` is because we are using 8-bit
 * timers).  Setting `OCR0A` and `OCR0B` controls the duty cycle of the PWM
 * output.
 */
void setup() {
    // pwm frequency is 967Hz
    // 16000000 / (64 * 256) = 976.5625 Hz

    // fast PWM (WGM = 0b011) with output on OC0A (PORTD6) and OC0B (PORTD5)
    TCCR0A = _BV(WGM00) | _BV(WGM01) | _BV(COM0A1) | _BV(COM0B1);
    // clk/64 prescaler (CS = 0b011)
    TCCR0B = _BV(CS01)  | _BV(CS00);

    // configure motor pins as outputs
    MOTORDDR |= MLEFTDIR | MRIGHTDIR | MLEFTPWM | MRIGHTPWM;

    // configure inputs
    JOYDDR &= ~(PIN_UP | PIN_DOWN | PIN_LEFT | PIN_RIGHT);
    // configure pullups
    JOYPORT  |= (PIN_UP | PIN_DOWN | PIN_LEFT | PIN_RIGHT);

    LEDDDR |= LEDPIN;
}

/**
 * Handle distance measurements with the ultrasonic sensor.
 *
 * The ultrasonic sensor on the mbot is a three-pin sensor: `GND`, `5V`,
 * and a data pin that is used as both an input and an output. We send a 10us
 * high pulse to the sensor to trigger a measurement. The sensor then sends out
 * a series of ultrasonic pulses, and then reports the measurement by
 * bringing the data line high.  The duration of the high pulse
 * corresponds to the distance.
 *
 * In order to time the high pulse, we simply count loop iterations.
 * This means that as you make changes to the code, you may need to
 * adjust the `STOP_DISTANCE` and `SLOW_DISTANCE` settings.
 */
uint8_t measure_distance(uint16_t *distance) {
    static DISTANCE_STATE state = PING;
    static uint16_t counter     = 0;
    bool valid                  = false;

    // Count main loop iterations. This is a static variable so it
    // preserves its value between calls.
    counter++;

    switch (state) {

        case PING:
            counter = 0;
            USDDR |= USPIN;         // configure ultrasonic sensor pin as output
            USPORTREG &= ~(USPIN);  // set low
            _delay_us(2);
            USPORTREG |= USPIN;     // set high
            _delay_us(10);
            USPORTREG &= ~(USPIN);  // set low
            USDDR &= ~(USPIN);      // configure us sensor pin as input
            state = WAIT_PULSE_START;
            break;

        case WAIT_PULSE_START:
            // when signal goes high, start counter and move to state 2.
            if (USPINREG & USPIN) {
                counter = 0;
                state = WAIT_PULSE_END;
            }
            break;

        case WAIT_PULSE_END:
            // when signal goes low, record counter, then move to state 3
            if ((USPINREG & USPIN) == 0) {
                *distance = counter;
                valid = true;
                state = INTERVAL;
                counter = 0;
            }
            break;

        case INTERVAL:
            // wait before making next measurement
            if (counter > 2000) state = PING;
            break;
    }

    return valid;
}

int main() {
    DIRECTION dir = STOPPED;
    TURN      turn = NONE;
    uint8_t   r_speed = 0,
              l_speed = 0;
    uint8_t   obstacle;
    uint8_t   max_speed = MAX_SPEED;
    uint16_t  distance;

    char      buf[40];

    setup();

    while (1) {
        /*
         * handle obstacle detection
         */

        if (measure_distance(&distance)) {
            if (distance < STOP_DISTANCE) {
                obstacle = 1;
            } else if (distance < SLOW_DISTANCE) {
                obstacle = 0;
                max_speed = MED_SPEED;
            } else {
                obstacle = 0;
                max_speed = MAX_SPEED;
            }
        }

        // Turn on LED when we detect an obstacle
        if (obstacle) {
            LEDPORTREG |= LEDPIN;
        } else {
            LEDPORTREG &= ~LEDPIN;
        }

        /*
         * handle joystick
         */

        if (!(JOYPIN & PIN_UP)) {
            dir = FORWARD;
        } else if (!(JOYPIN & PIN_DOWN)) {
            dir = REVERSE;
        } else {
            dir = STOPPED;
        }

        if (!(JOYPIN & PIN_LEFT)) {
            turn = LEFT;
        } else if (!(JOYPIN & PIN_RIGHT)) {
            turn = RIGHT;
        } else {
            turn = NONE;
        }

        // stop if we've detected an obstacle
        if ((dir == FORWARD) && obstacle) {
            dir = STOPPED;
        }

        /*
         * control motors
         */

        if (dir == STOPPED) {
            l_speed = r_speed = 0;
        } else {
            l_speed = r_speed = max_speed;

            if (dir == FORWARD) {
                MOTORPORT |= MLEFTDIR;
                MOTORPORT &= ~MRIGHTDIR;
            } else {
                MOTORPORT &= ~MLEFTDIR;
                MOTORPORT |= MRIGHTDIR;
            }
        }

        if (turn == RIGHT) {
            if (dir != STOPPED) {
                r_speed /= 2;
            } else {
                r_speed = l_speed = max_speed;
                MOTORPORT &= ~MLEFTDIR;
                MOTORPORT &= ~MRIGHTDIR;
            }
        } else if (turn == LEFT) {
            if (dir != STOPPED) {
                l_speed /= 2;
            } else {
                r_speed = l_speed = max_speed;
                MOTORPORT |= MLEFTDIR;
                MOTORPORT |= MRIGHTDIR;
            }
        }

        MLEFTOCR = l_speed;
        MRIGHTOCR = r_speed;
    }
}
