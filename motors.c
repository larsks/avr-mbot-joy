#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "serial.h"

#define MAX_SPEED  200
#define MED_SPEED  127
#define SLOW_SPEED 64

#define STOP_DISTANCE 150
#define SLOW_DISTANCE 500

#define LEDDDR     (DDRB)
#define LEDPORTREG (PORTB)
#define LEDPINREG  (PINB)
#define LEDPIN     (_BV(PORTB5))

#define USDDR     (DDRC)
#define USPORTREG (PORTC)
#define USPINREG  (PINC)
#define USPIN     (_BV(PORTC1))
#define USPCINT   (PCINT8)

#define MOTORDDR  (DDRD)
#define MOTORPORT (PORTD)
#define MLEFTDIR  (_BV(PORTD4))
#define MLEFTPWM  (_BV(PORTD5))
#define MRIGHTDIR (_BV(PORTD7))
#define MRIGHTPWM (_BV(PORTD6))
#define MLEFTOCR  (OCR0A)
#define MRIGHTOCR (OCR0B)

#define JOYDDR    (DDRB)
#define JOYPORT   (PORTB)
#define JOYPIN    (PINB)
#define PIN_LEFT  (_BV(PORTB1))
#define PIN_RIGHT (_BV(PORTB2))
#define PIN_DOWN  (_BV(PORTB3))
#define PIN_UP    (_BV(PORTB4))

typedef enum DIRECTION {STOPPED, FORWARD, REVERSE} DIRECTION;
typedef enum TURN {NONE, LEFT, RIGHT} TURN;
typedef enum DISTANCE_STATE {
    PING,
    WAIT_PULSE_START,
    WAIT_PULSE_END,
    INTERVAL
} DISTANCE_STATE;

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

uint8_t measure_distance(uint16_t *distance) {
    static DISTANCE_STATE state = PING;
    static uint16_t counter     = 0;
    bool valid                  = false;

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
    serial_begin();

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
