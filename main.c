#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>
#include "uart.h"
#include "xitoa.h"
//#include "ir_ctrl.h"


#define IR_TX_ON()		TCCR0A |= _BV(COM0B1) 	/* Tx: Start IR burst. means 1 */
#define IR_TX_OFF()		TCCR0A &= ~_BV(COM0B1)	/* Tx: Stop IR burst. means 0 */
#define ISR_COMPARE()   ISR(TIMER1_COMPA_vect)  /* Timer compare match ISR */

static void (*tick_func)(void);
ISR_COMPARE(){
	(*tick_func)();
}
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
void init_timer0(){
	DDRD = _BV(DDD6) | _BV(DDD5);
	PORTD &= ~_BV(PORTD5);
	TCCR0A = _BV(WGM01) | _BV(WGM00);
	TCCR0B = _BV(WGM02) | 0b010;
	OCR0A = 32;
	OCR0B = 10;
	TCNT0 = 0;
}

static void tick(void){
	PORTD ^= _BV(PORTD6);
}
void init_ticker(void (*func)(void)){
	// timer1 initialize
	// CTC mode and clk/1
	TCCR1B = _BV(WGM12) | _BV(CS10);

	// interval 435us
	// @10MHz -> OCR1A = 4348
	// @20MHz -> OCR1A = 8698
	OCR1A = 4348;
	TCNT1 = 0;

	tick_func = *func;		// set ticking function
	TIMSK1 |= _BV(OCIE1A);	// Enabnle compare interrupt
	sei();
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
				init_ticker(&tick);
				break;
			case 'v':
				xputs(PSTR("PD6 invert\n"));
				PORTD ^= _BV(PORTD6);
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

