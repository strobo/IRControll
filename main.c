#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>
#include "uart.h"
#include "xitoa.h"



static
void IoInit ()
{
	//PORTA = 0b11111111;	// Port A
	//PORTB = 0b10110000; // Port B
	PORTB = 0xff;
	//DDRB  = 0b11000000;
	DDRB = 0xff;

	PORTC = 0b11111111;	// Port C
	PORTD = 0b11111111; // Port D

	uart_init();		// Initialize UART driver
	sei();
}

static
void get_line (char *buff, int len)
{
	char c;
	int idx = 0;


	xputc('>');
	for (;;) {
		if (!uart_test()) continue;
		c = uart_get();
		if (c == '\r') break;
		if ((c == '\b') && idx) {
			idx--; xputc(c);
		}
		if (((uint8_t)c >= ' ') && (idx < len - 1)) {
			buff[idx++] = c; xputc(c);
		}
	}
	buff[idx] = 0;
	xputc('\n');
}


/*-----------------------------------------------------------------------*/
/* Main                                                                  */

int main (void)
{
	char line[64], *p;

	IoInit();
	/* Join xitoa module to uart module */
	xfunc_out = (void (*)(char))uart_put;

	xputs(PSTR("ir test monitor for AVR\n"));

	for (;;) {
		get_line(line, sizeof(line));
		p = line;

		switch(*p++){
			case 'n':
			xputs(PSTR("aaa\n"));
			break;
			case 's':
			xputs(PSTR("i\n"));
			break;
		}
	}
}

