#include "ir.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include "uart.h"
#include "xitoa.h"
#include <avr/pgmspace.h>

#define IR_TX_ON()		TCCR0A |= _BV(COM0B1) 	/* Tx: Start IR burst. means 1 */
#define IR_TX_OFF()		TCCR0A &= ~_BV(COM0B1)	/* Tx: Stop IR burst. means 0 */
#define ISR_COMPARE()   ISR(TIMER1_COMPA_vect)  /* Timer compare match ISR */

ISR_COMPARE(){
	(*tick_func)();
}

void tick(void){
	PORTD ^= _BV(PORTD6);

	switch(work.state) {
		case Idle:
			/* idle code */
			work.bitcount = 0;
			work.starter = 0;
			work.leader = 0;
			work.data = 0;
			work.trailer = 0;
			work.wait = 0;
			break;
		case Starter:
			/* Starter */
			if(data.format == DAIKIN) {
				if(work.starter < tus.STARTER_HEAD) {
					if(0 == (work.starter % 2)) {
						IR_TX_ON();
					} else {
						IR_TX_OFF();
					}
				} else {
					IR_TX_OFF();
				}
				work.starter++;
				if((tus.STARTER_HEAD + tus.STARTER_TAIL) <= work.starter) {
					work.state = Leader;
				}
			}
			break;
		case Leader:
			/* Leader */
			if(data.format == DAIKIN) {
				if(work.leader < tus.LEADER_DAIKIN_HEAD) {
					IR_TX_ON();
				} else {
					IR_TX_OFF();
				}
				work.leader++;
				if((tus.LEADER_DAIKIN_HEAD + tus.LEADER_DAIKIN_TAIL) <= work.leader) {
					work.state = Data;
					work.leader = 0;
				}
			}
			break;
		case Data:
			/* Data */
			if(data.format == DAIKIN) {
				if(work.data == 0) {
					IR_TX_ON();
					work.data++;
				} else {
					IR_TX_OFF();

					if(0 != (data.buffer[work.bitcount /8] & (1 << work.bitcount % 8))) {
						if( 3 <= work.data) {
							work.bitcount++;
							work.data = 0;
						} else {
							work.data++;
						}
					} else {
						if(1 <= work.data) {
							work.bitcount++;
							work.data = 0;
						} else {
							work.data++;
						}
					}
				}
				if (((work.bitcount == 8*8) || (work.bitcount == 16*8) || (work.bitcount == 35*8))
						&&work.data == 0) {    // sometimes not be add bitcount 
					work.state = Trailer;
				}
			}
			break;
		case Trailer:
			if (data.format == DAIKIN) {

				if (work.trailer < tus.TRAILER_DAIKIN_HEAD) {
					IR_TX_ON();
				} else {
					IR_TX_OFF();
				}
				work.trailer++;
				if ((tus.TRAILER_DAIKIN_HEAD + tus.TRAILER_DAIKIN_TAIL) <= work.trailer) {
					work.state = Wait;
					work.trailer = 0;
					if (work.bitcount == 35*8) {
						work.state = Idle;
					}
				}
			}
			break;
		case Wait:
			if (work.wait < tus.WAIT) {
				IR_TX_OFF();
			}
			work.wait++;
			if (tus.WAIT <= work.wait) {
				work.state = Leader;
				work.wait = 0;
			}
			break;
	}
}

void init_timer0(){
	DDRD = _BV(DDD6) | _BV(DDD5);
	PORTD &= ~_BV(PORTD5);
	TCCR0A = _BV(WGM01) | _BV(WGM00);
	TCCR0B = _BV(WGM02) | 0b010;
	OCR0A = 32;
	OCR0B = 10;
	//OCR0A = 16;
	//OCR0B = 5;
	TCNT0 = 0;
}

void init_ticker(void (*func)(void)){
	// timer1 initialize
	// CTC mode and clk/1
	TCCR1B = _BV(WGM12) | _BV(CS10);

	// interval 435us
	// @10MHz -> OCR1A = 4348
	// @20MHz -> OCR1A = 8698
	OCR1A = 4290; // 4348
	TCNT1 = 0;

	tick_func = *func;		// set ticking function
	TIMSK1 |= _BV(OCIE1A);	// Enabnle compare interrupt
	sei();
}

void init_ir(void)
{
	//tx.write(0.0);
	//init_ticker(&tick);
	init_timer0();
	//tus_daikin = 435;


	work.state = Idle;
	work.bitcount = 0;
	work.starter = 0;
	work.leader = 0;
	work.data = 0;
	work.trailer = 0;

	data.format = UNKNOWN;
	data.bitlength = 0;

	tus.STARTER_HEAD = 12;
	tus.STARTER_TAIL = 58;	// 57
	tus.LEADER_DAIKIN_HEAD = 8;
	tus.LEADER_DAIKIN_TAIL = 4;

	tus.TRAILER_DAIKIN_HEAD = 1;
	tus.TRAILER_DAIKIN_TAIL = 2;

	tus.WAIT = 78; //78  79.05ms
}

uint16_t setData(IR_Format format , uint8_t *buf, uint16_t bitlength)
{
	char i;
	////LOCK();
	if (work.state != Idle) {
	//	UNLOCK();
		return -1;
	}

	work.state = Starter;
	work.bitcount = 0;
	work.starter = 0;
	work.leader = 0;
	work.data = 0;
	work.wait = 0;
	work.trailer = 0;

	data.format = format;
	data.bitlength = bitlength;
	for (i = 0; i < 35; i++) {
		data.buffer[i] = buf[i];
		xprintf(PSTR("0x%02X "), buf[i]);
	}


	switch (format) {
	//	case NEC:
	//		//ticker.detach();
	//		//ticker.attach_us(this, &TransmitterIR::tick, RemoteIR::TUS_NEC);
	//		break;
		case DAIKIN:
	//		//ticker.detach();
			init_ticker(&tick);
	//		//ticker.attach_us(this, &TransmitterIR::tick, RemoteIR::TUS_DAIKIN);
			break;
	}

	//UNLOCK();
	return bitlength;
}

