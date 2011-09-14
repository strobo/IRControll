#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>
#include "uart.h"
#include "xitoa.h"
#include "ir.h"


#define IR_TX_ON()		TCCR0A |= _BV(COM0B1) 	/* Tx: Start IR burst. means 1 */
#define IR_TX_OFF()		TCCR0A &= ~_BV(COM0B1)	/* Tx: Stop IR burst. means 0 */

static
void IoInit ()
{
	//PORTA = 0b11111111;	// Port A
	//PORTB = 0b10110000; // Port B
	PORTB = 0xff;
	//DDRB  = 0b11000000;
	DDRB = 0xff;

	PORTC = 0b11111111;	// Port C
	PORTD = 0b00000000; // Port D

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

uint8_t atoi16(  char *NumberString )
{
	return (uint8_t)strtoul( NumberString, NULL, 16 );
}
uint8_t atoi2( char *NumberString )
{
	uint8_t    lData = 0;
	uint8_t    cbPick;

	do{
		cbPick = (uint8_t)*NumberString;
		if((cbPick == '0') || (cbPick == '1')){
			cbPick &= 0x0f;
			lData = lData * 2 + (uint8_t)cbPick;
		}
	}while(*NumberString++ != '\0');
	return lData;
}

/*-----------------------------------------------------------------------*/
/* Main                                                                  */

int main (void)
{
	char line[64], *p;
	uint8_t buf[35];
	uint8_t *ramAddr,val;
	char *addr_s,*val_s;
	buf[0]=0x11;buf[1]=0xDA;buf[2]=0x27;buf[3]=0x00;buf[4]=0xC5;buf[5]=0x00;buf[6]=0x00;buf[7]=0xD7;
	buf[8]=0x11;buf[9]=0xDA;buf[10]=0x27;buf[11]=0x00;buf[12]=0x42;buf[13]=0x00;buf[14]=0x00;buf[15]=0x54;
	buf[16]=0x11;buf[17]=0xDA;buf[18]=0x27;buf[19]=0x00;buf[20]=0x00;buf[21]=0x39;buf[22]=0x28;buf[23]=0x00;
	buf[24]=0xA0;buf[25]=0x00;buf[26]=0x00;buf[27]=0x06;buf[28]=0x60;buf[29]=0x00;buf[30]=0x00;buf[31]=0xC1;
	buf[32]=0x00;buf[33]=0x00;buf[34]=0x3A;
	IoInit();
	/* Join xitoa module to uart module */
	xfunc_out = (void (*)(char))uart_put;

	xputs(PSTR("ir test monitor for AVR\n"));

	for (;;) {
		get_line(line, sizeof(line));
		p = line;

		switch(*p++){
			case 'b':
				xputs(PSTR("brust38k!\n"));
				init_timer0();
				IR_TX_ON();
				break;
			case 'n':
				xputs(PSTR("TX_OFF\n"));
				IR_TX_OFF();
				break;
			case 'c':
				xputs(PSTR("Tick setup\n"));
				init_ticker((void (*)())tick);
				break;
			case 'v':
				xputs(PSTR("PD6 invert\n"));
				PORTD ^= _BV(PORTD6);
				break;
			case 's':
				xputs(PSTR("setData()\n"));
				init_ir();
				setData(DAIKIN, buf, 35*8);
				break;
			case 'w':	/* w <addr> <val> addrにvalを書きこむ */
			if(*p++ == 32) {
				addr_s = strtok(p, " ");
				val_s  = strtok(NULL, " ");

				ramAddr = (char *)atoi16(addr_s);
				if(strncmp(val_s, "0b",2) == 0){
					val_s += 2;
					val = (uint8_t)atoi2(val_s);
				}else if(strncmp(val_s, "0x", 2) == 0){
					val_s += 2;
					val = (uint8_t)atoi16(val_s);
				}

				xprintf(PSTR("ramAddr: %d\n"), ramAddr);
				xprintf(PSTR("val: %d\n"), val);
				*ramAddr = val;
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

