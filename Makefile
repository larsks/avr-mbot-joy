PROGNAME ?= mbot-joy

DEVICE         ?= atmega328p
AVR_PORT       ?= /dev/ttyUSB4
AVR_PROGRAMMER ?= arduino
CLOCK          ?= 16000000

PORT = -P $(AVR_PORT) $(if $(AVR_SPEED), -b $(AVR_SPEED))

OBJDUMP    ?= avr-objdump
OBJCOPY    ?= avr-objcopy
DEBUGFLAGS ?= -Os
CC         = avr-gcc
CFLAGS = -mmcu=$(DEVICE) -DF_CPU=$(CLOCK) $(DEBUGFLAGS)

AVRDUDE = avrdude $(PORT) -p $(DEVICE) -c $(AVR_PROGRAMMER)

OBJS = \
	   $(PROGNAME).o

%.s: %.c
	$(CC) $(CFLAGS) -o $@ -S $<

all: $(PROGNAME).hex

$(PROGNAME).elf: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(PROGNAME).hex: $(PROGNAME).elf
	rm -f $(PROGNAME).hex
	$(OBJCOPY) -j .text -j .data -O ihex $(PROGNAME).elf $(PROGNAME).hex
	avr-size --format=avr --mcu=$(DEVICE) $(PROGNAME).elf

flash: $(PROGNAME).hex
	$(AVRDUDE) -U flash:w:$(PROGNAME).hex:i

check:
	$(AVRDUDE)

clean:
	rm -f $(OBJS) $(PROGNAME).hex $(PROGNAME).elf
