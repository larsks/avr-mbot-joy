/**
 * \file serial.c
 */
#include <avr/io.h>

#ifndef BAUD
//! Selected serial baud rate. This must be set before including
//! `util/setbaud.h` because it is used in that file to calculate timings.
#define BAUD 9600
#endif

#include <util/setbaud.h>

//! Configure uart
void serial_begin() {
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); // 8-bit data
    UCSR0B = _BV(RXEN0)  | _BV(TXEN0);  // Enable RX and TX
}

//! Output a single character. This waits for any in progress transmission
//! to complete before putting a new character into `UDR0`.
void serial_putchar(char c) {
    loop_until_bit_is_set(UCSR0A, UDRE0); /* Wait until data register empty. */
    UDR0 = c;
}

//! Print a string to the serial port
void serial_print(char *s) {
    while (*s) serial_putchar(*s++);
}

//! Print a string to the serial port, followed by a CR/LF pair
void serial_println(char *s) {
    serial_print(s);
    serial_putchar('\r');
    serial_putchar('\n');
}
