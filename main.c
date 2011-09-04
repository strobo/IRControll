#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>
#include "uart.h"
#include "xitoa.h"
#include "ir_ctrl.h"


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

unsigned int atoi16(  char *NumberString )
{
	unsigned int result = strtoul( NumberString, NULL, 16 );
	return result;
}

/*-----------------------------------------------------------------------*/
/* Main                                                                  */

int main (void)
{
	char line[64], *p;
	uint8_t *ramAddr,val;
	char *addr_s,*val_s;

	IoInit();
	/* Join xitoa module to uart module */
	xfunc_out = (void (*)(char))uart_put;

	xputs(PSTR("ir test monitor for AVR\n"));

	for (;;) {
		get_line(line, sizeof(line));
		p = line;

		switch(*p++){
			case 'b':
			xputs(PSTR("brust38k!"));
			//brust38k();
			TCCR0A = _BV(WGM01) | _BV(WGM00);
			TCCR0B = _BV(WGM02) | 0b010;
			OCR0A = 32;
			TCNT0 = 0;
			TCCR0A |= _BV(COM0B1);

			break;
			case 'w':	/* w <addr> <val> addrにvalを書きこむ */
			if(*p++ == 32) {
				addr_s = strtok(p, " ");
				val_s  = strtok(NULL, " ");
				ramAddr = (char *)atoi16(addr_s);
				val     = atoi16(val_s);

				xprintf(PSTR("addr_s is: %s\n"),addr_s);
				xprintf(PSTR("val_s is: %s\n"),val_s);
				xprintf(PSTR("ramAddr: %d\n"), ramAddr);
				xprintf(PSTR("val: %d\n"), val);
				*ramAddr = val;
				ramAddr = val = addr_s = val_s = 0;
			}
			break;
			case 'r':	/* r <addr> addrのデータを読み出す */
			if(*p++ == 32) {
				addr_s = p;
				ramAddr = (char *)atoi16(addr_s);

				xprintf(PSTR("ramAddr: %d\n"), ramAddr);
				xprintf(PSTR("val: %d\n"), *ramAddr);
				ramAddr = addr_s = 0;
			}
			break;
		}
	}
}

