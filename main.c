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



/*-----------------------------------------------------------------------*/
/* Main                                                                  */

int main (void)
{
	IoInit();
	/* Join xitoa module to uart module */
	xfunc_out = (void (*)(char))uart_put;

	xputs(PSTR("ir test monitor for AVR\n"));

	for (;;) {
		xputc(uart_get());
	}
}

