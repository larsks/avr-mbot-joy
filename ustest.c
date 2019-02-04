#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "serial.h"

#define USPIN       (_BV(PORTC1))
#define DEBUG1PIN   (_BV(PORTB4))
#define DEBUG2PIN   (_BV(PORTB3))

int main() {
    uint16_t counter0, counter1;
    char buf[40];

    serial_begin();

    DDRB |= DEBUG1PIN|DEBUG2PIN;

    while (1) {
        serial_println("start");

        PORTB |= DEBUG1PIN;

        DDRC |= USPIN;
        DDRB |= DEBUG1PIN;
        PORTC &= ~(USPIN);  // bring uspin low for 2us
        _delay_us(2);
        PORTC |= USPIN;     // bring uspin high for 10us
        _delay_us(10);
        PORTC &= ~USPIN;    // bring uspin low, switch to input
        DDRC &= ~(USPIN);

        PORTB |= DEBUG2PIN;

        counter0 = 0;
        while (!(PINC & USPIN)) {  // wait for uspin to go high;
            if (counter0++ > 30000) break;
            _delay_us(1);
        }

        PORTB &= ~DEBUG1PIN;

        counter1 = 0;
        while (PINC & USPIN) {     // wait for uspin to go low
            if (counter1++ > 30000) break;
            _delay_us(1);
        }

        PORTB &= ~DEBUG2PIN;

        sprintf(buf, "measured: %d %d", counter0, counter1);
        serial_println(buf);
        _delay_ms(2000);
    }
}
