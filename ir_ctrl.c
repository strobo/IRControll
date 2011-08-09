/*----------------------------------------------------------------------------/
/  IR-CTRL - IR remote control module  R0.01                  (C)ChaN, 2008
/-----------------------------------------------------------------------------/
/ The IR_CTRL is a generic Transmisson/Reception control module for IR remote
/ control systems. This is a free software and is opened for education,
/ research and development under license policy of following trems.
/
/  Copyright (C) 2008, ChaN, all right reserved.
/
/ * The IR_CTRL module is a free software and there is no warranty.
/ * You can use, modify and/or redistribute it for personal, non-profit or
/   commercial use without restriction under your responsibility.
/ * Redistributions of source code must retain the above copyright notice.
/
/-----------------------------------------------------------------------------/
/ Aug 30,'08 R0.01  First release.
/----------------------------------------------------------------------------*/

#include "ir_ctrl.h"


/*----------------------------------------------------------------------------/
/ Platform dependent definitions
/----------------------------------------------------------------------------*/
/* Define interrupt service functions */
#define	ISR_COMPARE()	<to be filled>	/* Timer compare match ISR */
#define	ISR_CAPTURE()	<to be filled>	/* Rx: Timer input capture ISR */

/* Define hardware control macros */
#define	IR_INIT_TIMER()	<to be filled>	/* Initialize Timer (Timer1 for transmission/reception timing: Free running, clk/8) */
#define	IR_INIT_XMIT()	<to be filled>	/* Tx: Initialize Transmitter (Timer0 for IR subcarrier: Fast PWM, clk/8) */

#define IR_TX_38K()		<to be filled>	/* Tx: Set IR burst frequency to 38kHz */
#define IR_TX_40K()		<to be filled>	/* Tx: Set IR burst frequency to 40kHz */
#define IR_TX_ON()		<to be filled> 	/* Tx: Start IR burst */
#define IR_TX_OFF()		<to be filled>	/* Tx: Stop IR burst */
#define IR_TX_TEST()	<to be filled>	/* Tx: Check if IR is being transmitted or not */

#define IR_CAPT_TEST()	<to be filled>	/* Rx: Check which edge generated the capture interrupt */
#define IR_CAPT_RISE()	<to be filled>	/* Rx: Set captureing is triggered on rising edge */
#define IR_CAPT_FALL()	<to be filled>	/* Rx: Set captureing is triggered on falling edge */
#define IR_CAPT_ENA()	<to be filled>	/* Rx: Enable captureing interrupt */
#define	IR_CAPT_REG()	<to be filled>	/* Rx: Returns the value in capture register */
#define IR_CAPT_DIS()	<to be filled>	/* Tx && Rx: Disable captureing interrupt */

#define IR_COMP_ENA(n)	<to be filled>	/* Enable compare interrupt n count after now */
#define IR_COMP_DIS()	<to be filled>	/* Disable compare interrupt */
#define IR_COMP_NEXT(n)	<to be filled>	/* Tx: Increase compare register by n count */

/* Counter clock rate and register width */
#define T_CLK			<to be filled>	/* Timer tick period [ns] */
#define	_tmr_t			<to be filled>	/* Integer type of timer register */

/*---------------------------------------------------------------------------*/


/* IR control timings */
#define	T_NEC	(562000/T_CLK)		/* Base time for NEC format (T=562us) */
#define	T_AEHA	(425000/T_CLK)		/* Base time for AEHA format (T=425us) */
#define T_SONY	(600000/T_CLK)		/* Base time for SONY format (T=600us) */
#define T_TRAIL	(6000000/T_CLK)		/* Trailer detection time (6ms) */


/* Working area for IR communication  */

volatile IR_STRUCT IrCtrl;


/* IR receiving interrupt on either edge of input */
#if IR_USE_RCVR
ISR_CAPTURE()
{
	static _tmr_t pw1, pw2;	/* Pulse width counter */
	_tmr_t ct, pw;
	static uint8_t b;		/* Bit counter */
	uint8_t i, f, d;


	ct = IR_CAPT_REG();

	/* On stop of burst (rising edge) */
	if (IR_CAPT_TEST()) {
		IR_CAPT_FALL();			/* Next is start of carrier (falling edge on input) */
		IR_COMP_ENA(T_TRAIL);	/* Enable trailer timer */
		pw1 = ct - pw1;			/* pw1: carrier length */
		pw2 = ct;
		if (IR_USE_SONY && IrCtrl.fmt == SONY && pw1 >= (uint16_t)(T_SONY * 0.8) && pw1 <= (uint16_t)(T_SONY * 2.5)) {
			i = IrCtrl.phase / 8;
			if (i < sizeof(IrCtrl.buff)) {
				d = IrCtrl.buff[i];
				IrCtrl.buff[i] = (pw1 >= (uint16_t)(T_SONY * 1.5)) ? d | b : d & ~b;
				if ((b <<= 1) == 0) b = 1;
				IrCtrl.phase++;
			}
		}
		return;
	}

	/* On start of burst (falling edge) */
	IR_CAPT_RISE();							/* Next is stop of carrier (rising edge on input) */
	IR_COMP_DIS();							/* Disable trailer timer */
	pw = pw1; pw1 = ct; ct -= pw2;			/* pw: mark length, ct: space length */
	if (IrCtrl.state >= IR_RECVED) return;	/* Reject if not ready to receive */

	f = 0;
	if (IR_USE_NEC && pw >= T_NEC * 13 && pw <= T_NEC * 19) {		/* Is NEC leader pattern? */
 		if (ct >= T_NEC * 6 && ct <= T_NEC * 10) f = NEC;
		if (ct >= T_NEC * 3 && ct <= T_NEC * 5) f = NEC|REPT;
	}
	if (IR_USE_AEHA && pw >= T_AEHA * 5 && pw <= T_AEHA * 12) {		/* Is AEHA leader pattern? */
 		if (ct >= (uint16_t)(T_AEHA * 2.5) && ct <= (uint16_t)(T_AEHA * 5.5)) f = AEHA;
		if (ct >= T_AEHA * 5 && ct <= T_AEHA * 11) f = AEHA|REPT;
	}
	if (IR_USE_SONY && pw >= T_SONY * 3 && pw <= T_SONY * 5) {		/* Is SONY leader pattern? */
		if (ct >= (uint16_t)(T_SONY * 0.75) && ct <= (uint16_t)(T_SONY * 1.25)) f = SONY;
	}
	if (f) {	/* A leader pattern is detected */
		IrCtrl.fmt = f;
		IrCtrl.phase = 0;
		b = 1;
		IrCtrl.state = IR_RECVING;
		return;
	}

	if (IrCtrl.state == IR_RECVING) {
		i = IrCtrl.phase / 8;
		if (i >= sizeof(IrCtrl.buff)) return;

		d = IrCtrl.buff[i];
		f = IrCtrl.fmt;
		if (IR_USE_NEC && f == NEC && pw <= (uint16_t)(T_NEC * 1.5) && ct <= (uint16_t)(T_NEC * 3 * 1.5)) {	/* Is NEC data mark? */
			IrCtrl.buff[i] = (ct >= T_NEC * 2) ? d | b : d & ~b;
			if ((b <<= 1) == 0) b = 1;
			IrCtrl.phase++;
			return;
		}
		if (IR_USE_AEHA && f == AEHA && pw <= (uint16_t)(T_AEHA * 1.5) && ct <= (uint16_t)(T_AEHA * 3 * 1.5)) {	/* Is AEHA data mark? */
			IrCtrl.buff[i] = (ct >= T_AEHA * 2) ? d | b : d & ~b;
			if ((b <<= 1) == 0) b = 1;
			IrCtrl.phase++;
			return;
		}
		if (IR_USE_SONY && f == SONY && ct <= (uint16_t)(T_SONY * 1.5)) {		/* Is SONY data mark? */
			return;		/* Nothing to do at start of carrier */
		}
	}

	IrCtrl.state = IR_IDLE;	/* When an invalid mark width is detected, abort and return idle state */
}
#endif /* IR_USE_RCVR */



/* Transmission timing and Trailer detection */

ISR_COMPARE()
{
	uint8_t st = IrCtrl.state;

#if IR_USE_XMIT
	uint8_t i, d, f = IrCtrl.fmt;
	uint16_t w;

	if (st == IR_XMITING) {
		if (IR_TX_TEST()) {				/* End of mark? */
			IR_TX_OFF();				/* Stop burst */
			i = IrCtrl.phase;
			if (i < IrCtrl.len) {		/* Is there a bit to be sent? */
				if (IR_USE_SONY && (f & SONY)) {
					w = T_SONY;
				} else {
					i /= 8;
					d = IrCtrl.buff[i];
					if (IR_USE_AEHA && (f & AEHA))
						w = (d & 1) ? T_AEHA * 3 : T_AEHA;
					else
						w = (d & 1) ? T_NEC * 3 : T_NEC;
					IrCtrl.buff[i] = d >> 1;
				}
				IR_COMP_NEXT(w);
				return;
			}
		} else {
			IR_TX_ON();					/* Start burst */
			i = ++IrCtrl.phase / 8;
			if (IR_USE_SONY && (f & SONY)) {
				d = IrCtrl.buff[i];
				w = (d & 1) ? T_SONY * 2 : T_SONY;
				IrCtrl.buff[i] = d >> 1;
			} else {
				w = (f & NEC) ? T_NEC : T_AEHA;
			}
			IR_COMP_NEXT(w);
			return;
		}
	}

	if (st == IR_XMIT) {
		IR_TX_OFF();					/* Stop carrier */
		switch (f) {					/* Set next transition time */
#if IR_USE_SONY
		case SONY:
			w = T_SONY;
			break;
#endif
#if IR_USE_AEHA
		case AEHA:
			w = IrCtrl.len ? T_AEHA * 4 : T_AEHA * 8;
			break;
#endif
		default:	/* NEC */
			w = IrCtrl.len ? T_NEC * 8 : T_NEC * 4;
			break;
		}
		IR_COMP_NEXT(w);
		IrCtrl.state = IR_XMITING;
		IrCtrl.phase = 0xFF;
		return;
	}
#endif /* IR_USE_XMIT */

	IR_COMP_DIS();					/* Disable compare */

#if IR_USE_RCVR
#if IR_USE_XMIT
	IR_CAPT_ENA();					/* Re-enable receiving */
#endif
	if (st == IR_RECVING) {			/* Trailer detected */
		IrCtrl.len = IrCtrl.phase;
		IrCtrl.state = IR_RECVED;
		return;
	}
#endif

	IrCtrl.state = IR_IDLE;
}




/*---------------------------*/
/* Data Transmission Request */
/*---------------------------*/

#if IR_USE_XMIT
int IR_xmit (
	uint8_t fmt,			/* Frame format: NEC, AEHA or SONY */
	const uint8_t* data,	/* Pointer to the data to be sent */
	uint8_t len				/* Data length [bit]. 0 for a repeat frame */
)
{
	_tmr_t lw;
	uint8_t i;


	if (len / 8 > sizeof(IrCtrl.buff)) return 0;	/* Too long data */
	if (IrCtrl.state != IR_IDLE) return 0;			/* Abort when collision detected */

	switch (fmt) {
#if IR_USE_NEC
	case NEC:	/* NEC frame */
		if (len != 0 && len != 32) return 0;		/* Must be 32 bit data */
		lw = T_NEC * 16;	/* Leader burst time */
		IR_TX_38K();
		break;
#endif
#if IR_USE_AEHA
	case AEHA:	/* AEHA frame */
		if ((len > 0 && len < 48) || len % 8) return 0;	/* Must be 48 bit or longer data */
		lw = T_AEHA * 8;	/* Leader burst time */
		IR_TX_38K();
		break;
#endif
#if IR_USE_SONY
	case SONY:	/* SONY frame */
		if (len != 12 && len != 15 && len != 20) return 0;	/* Must be 12, 15 or 20 bit data */
		lw = T_SONY * 4;	/* Leader burst time */
		IR_TX_40K();
		break;
#endif
	default:
		return 0;
	}

#if IR_USE_RCVR
	IR_CAPT_DIS();
#endif
	IR_COMP_DIS();
	IrCtrl.fmt = fmt;
	IrCtrl.len = (IR_USE_SONY && (fmt == SONY)) ? len - 1 : len;
	len = (len + 7) / 8;
	for (i = 0; i < len; i++) IrCtrl.buff[i] = data[i];

	/* Start transmission sequense */
	IrCtrl.state = IR_XMIT;
	IR_TX_ON();
	IR_COMP_ENA(lw);

	return 1;
}
#endif /* IR_USE_XMIT */



/*---------------------------*/
/* Initialize IR functions   */
/*---------------------------*/

void IR_initialize (void)
{
	/* Initialize timer and port functions for IR communication */
	IR_INIT_TIMER();
#if IR_USE_XMIT
	IR_INIT_XMIT();
#endif

	IrCtrl.state = IR_IDLE;

	/* Enable receiving */
#if IR_USE_RCVR
	IR_CAPT_FALL();
	IR_CAPT_ENA();
#endif
}


