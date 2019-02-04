#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "serial.h"

#define USPIN (_BV(PORTC1))
#define LEDPIN (_BV(PORTB5))

int main() {
    int counter;
    char buf[40];

    serial_begin();

    while (1) {
        serial_println("start");
        DDRC |= USPIN;
        DDRB |= LEDPIN;
        PORTC &= ~(USPIN);  // bring portc1 low for 2us
        _delay_us(2);
        PORTC |= USPIN;     // bring portc1 high for 10us
        _delay_us(10);
        PORTC &= ~USPIN;    // bring portc1 low, switch to input
        DDRC &= ~(USPIN);

        PORTB |= LEDPIN;

        counter = 0;
        while (!(PORTC & USPIN)) {  // wait for portc1 to go high;
            if (counter++ > 30000) break;
        }
        counter = 0;
        while (PORTC &USPIN) {
            if (counter++ > 30000) break;
        }

        PORTB &= ~LEDPIN;

        sprintf(buf, "measured: %d", counter);
        serial_println(buf);
        _delay_ms(2000);
    }
}
