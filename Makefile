PROGNAME = blink

DEVICE = atmega328p
AVR_PORT = /dev/ttyUSB4
AVR_PROGRAMMER = arduino
CLOCK = 16000000

PORT = -P $(AVR_PORT) $(if $(AVR_SPEED), -b $(AVR_SPEED))

DEBUGFLAGS = -Os
CC = avr-gcc
OBJDUMP = avr-objdump
CFLAGS = -mmcu=$(DEVICE) -DF_CPU=$(CLOCK) $(DEBUGFLAGS)

AVRDUDE = avrdude $(PORT) -p $(DEVICE) -c $(AVR_PROGRAMMER)

OBJS = \
	   $(PROGNAME).o \
	   serial.o

all: $(PROGNAME).hex

$(PROGNAME).elf: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(PROGNAME).hex: $(PROGNAME).elf
	rm -f $(PROGNAME).hex
	avr-objcopy -j .text -j .data -O ihex $(PROGNAME).elf $(PROGNAME).hex
	avr-size --format=avr --mcu=$(DEVICE) $(PROGNAME).elf

flash: $(PROGNAME).hex
	$(AVRDUDE) -U flash:w:$(PROGNAME).hex:i

check:
	$(AVRDUDE)

clean:
	rm -f $(OBJS)
