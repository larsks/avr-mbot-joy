#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define LEDDDR DDRB
#define LEDPORTREG PORTB
#define LEDPINREG PINB
#define LEDPIN (1<<PORTB5)

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

typedef enum DIRECTION {FORWARD, REVERSE, STOPPED} DIRECTION;
typedef enum TURN {LEFT, RIGHT, NONE} TURN;

int main() {
    DIRECTION dir = STOPPED;
    TURN turn = NONE;
    uint8_t r_speed = 0, l_speed = 0;

    // pwm frequency is 967Hz
    // 16000000 / (64 * 256) = 976.5625 Hz
    TCCR0A = _BV(WGM00) | _BV(WGM01) | _BV(COM0A1) | _BV(COM0B1);// Fast PWM (WGM = 0b0101)
    TCCR0B = _BV(CS01) | _BV(CS00);                 // clk/64 prescaler (CS = 0b011)

    // configure motor pins as outputs
    DDRD |= MLEFTDIR | MRIGHTDIR | MLEFTPWM | MRIGHTPWM;
//    MOTORPORT = MLEFTDIR | MRIGHTDIR;

    // configure inputs
    JOYDDR &= ~(PIN_UP|PIN_DOWN|PIN_LEFT|PIN_RIGHT);
    // configure pullups
    JOYPORT |= (PIN_UP|PIN_DOWN|PIN_LEFT|PIN_RIGHT);

    while (1) {
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

        if (dir == STOPPED) {
            l_speed = r_speed = 0;
        } else {
            l_speed = r_speed = 127;

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
                r_speed = l_speed = 127;
                MOTORPORT &= ~MLEFTDIR;
                MOTORPORT &= ~MRIGHTDIR;
            }
        } else if (turn == LEFT) {
            if (dir != STOPPED) {
                l_speed = 64;
            } else {
                r_speed = l_speed = 127;
                MOTORPORT |= MLEFTDIR;
                MOTORPORT |= MRIGHTDIR;
            }
        }

        MLEFTOCR = l_speed;
        MRIGHTOCR = r_speed;
    }
        
}
