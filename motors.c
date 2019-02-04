#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "serial.h"

#define LEDDDR (DDRB)
#define LEDPORTREG (PORTB)
#define LEDPINREG (PINB)
#define LEDPIN (1<<PORTB5)

#define USDDR (DDRC)
#define USPORTREG (PORTC)
#define USPINREG (PINC)
#define USPIN (1<<PORTC1)
#define USPCINT (PCINT8)

#define MOTORDDR (DDRD)
#define MOTORPORT (PORTD)
#define MLEFTDIR (1<<PORTD4)
#define MLEFTPWM (1<<PORTD5)
#define MRIGHTDIR (1<<PORTD7)
#define MRIGHTPWM (1<<PORTD6)
#define MLEFTOCR (OCR0A)
#define MRIGHTOCR (OCR0B)

#define JOYDDR (DDRB)
#define JOYPORT (PORTB)
#define JOYPIN (PINB)
#define PIN_LEFT (1<<PORTB1)
#define PIN_RIGHT (1<<PORTB2)
#define PIN_DOWN (1<<PORTB3)
#define PIN_UP (1<<PORTB4)

typedef enum DIRECTION {STOPPED, FORWARD, REVERSE} DIRECTION;
typedef enum TURN {NONE, LEFT, RIGHT} TURN;

int main() {
    DIRECTION dir = STOPPED;
    TURN turn = NONE;
    uint8_t r_speed = 0, l_speed = 0;
    uint8_t wait_for_echo = 0;
    uint16_t counter;
    uint8_t obstacle;
    uint8_t max_speed = 200;

    char buf[40];

    serial_begin();

    // pwm frequency is 967Hz
    // 16000000 / (64 * 256) = 976.5625 Hz

    // Fast PWM (WGM = 0b0101) with output on 
    TCCR0A = _BV(WGM00) | _BV(WGM01) | _BV(COM0A1) | _BV(COM0B1);
    TCCR0B = _BV(CS01) | _BV(CS00);                 // clk/64 prescaler (CS = 0b011)

    // configure motor pins as outputs
    MOTORDDR |= MLEFTDIR | MRIGHTDIR | MLEFTPWM | MRIGHTPWM;

    // configure inputs
    JOYDDR &= ~(PIN_UP|PIN_DOWN|PIN_LEFT|PIN_RIGHT);
    // configure pullups
    JOYPORT |= (PIN_UP|PIN_DOWN|PIN_LEFT|PIN_RIGHT);

    LEDDDR |= LEDPIN;

    while (1) {
        counter++;

        if (wait_for_echo == 1) {
            if (USPINREG & USPIN) {
                wait_for_echo = 2;
            }
        } else if (wait_for_echo == 2) {
            if ((USPINREG & USPIN) == 0) {
                sprintf(buf, "distance: %u", counter);
                serial_println(buf);
                wait_for_echo = 3;
                if (counter < 400) {
                    obstacle = 1;
                    LEDPORTREG |= LEDPIN;
                } else {
                    obstacle = 0;
                    LEDPORTREG &= ~LEDPIN;
                    if (counter < 1000) {
                        max_speed = 127;
                    } else { 
                        max_speed = 200;
                    }
                }
                counter = 0;
            }
        } else if (wait_for_echo == 3) {
            if (counter > 500) wait_for_echo = 0;
        } else {
            serial_println("send ping");
            counter = 0;
            USDDR |= USPIN;         // configure ultrasonic sensor pin as output
            USPORTREG &= ~(USPIN);  // set low
            _delay_us(2);
            USPORTREG |= USPIN;     // set high
            _delay_us(10);
            USPORTREG &= ~(USPIN);  // set low
            USDDR &= ~(USPIN);      // configure us sensor pin as input
            wait_for_echo = 1;
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

        if ((dir == FORWARD) && obstacle) {
            dir = STOPPED;
        }

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
                r_speed = 64;
            } else {
                r_speed = l_speed = max_speed;
                MOTORPORT &= ~MLEFTDIR;
                MOTORPORT &= ~MRIGHTDIR;
            }
        } else if (turn == LEFT) {
            if (dir != STOPPED) {
                l_speed = 64;
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
