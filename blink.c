#include <avr/io.h>
#include <util/delay.h>

#define LEDPIN (1<<PORTB5)

int main() {
    DDRB |= LEDPIN;

    while(1) {
        PINB = LEDPIN;
        _delay_ms(2000);
    }
}
