#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <string.h>
#include <ctype.h>
#include "uart.h"
#include "xitoa.h"
//#include "ntshell/ntshell.h"
//#include "ntshell/ntopt.h"

//#include "ir_ctrl.h"
/*
typedef struct {
	char *command;
	char *description;
	void (*func)(int argc, char **argv);
} command_table_t;

void cmd_foo();
void cmd_foo(int argv, char **argc)
{
	//homu
}

const command_table_t cmdlist[] = {
	{"foo", "foo command", cmd_foo },
	{NULL, NULL, NULL}
};

int func_read(char *buf, int cnt);
int func_write(const char *buf, int cnt);
int func_cb_ntshell(const char *text);
void func_cb_ntopt(int argc, char **argv);


int func_read(char *buf, int cnt) {
	for (int i = 0; i < cnt; i++) {
		buf[i] = uart_get();
	}
	return 0;
}

int func_write(const char *buf, int cnt) {
	for (int i = 0; i < cnt; i++) {
		uart_put(buf[i]);
	}
	return 0;
}

int func_cb_ntshell(const char *text) {
	return ntopt_parse(text, func_cb_ntopt);
}
void func_cb_ntopt(int argc, char **argv) {
	if (argc == 0) {
		return;
	}    
	int execnt = 0;
	const command_table_t *p = &cmdlist[0];
	while (p->command != NULL) {
		if (strcmp(argv[0], p->command) == 0) {
			p->func(argc, argv);
			execnt++;
		}
		p++;
	}
	if (execnt == 0) {
		xprintf(PSTR("Command not found.\n")); // PSTR?
	}
}*/
int main (void)
{
	//vtparse_t parser;
	//text_editor_t editor;
	//text_history_t history;

	PORTB = 0b00100111; 	/* PortB, IR input */
	DDRB  = 0b00000000;
	PORTD = 0b11011111;		/* PortD, IR drive, Comm */
	DDRD  = 0b00100010;
	PORTC = 0b00111111;		/* PortC */
	DDRC  = 0b00000000;

	uart_init();			/* Initialize UART driver */
	xfunc_out = (void(*)(char))uart_put;	/* Join xitoa module to communication module */
	sei();

	xputs(PSTR("IR remote control test program\n"));

	//ntshell_execute(&parser,&editor,&history, func_read, func_write, func_cb_ntshell);
	for(;;){
		xputc("a");
		}
}
