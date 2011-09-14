#ifndef _IR_
#define _IR_

#include <avr/pgmspace.h>
static void (*tick_func)(void);
typedef enum {
	UNKNOWN,
	NEC,
	DAIKIN
} IR_Format;

typedef enum {
	Idle,
	Starter,
	Leader,
	Data,
	Wait,
	Trailer
} State;

typedef struct {
	State state;
	int bitcount;
	int starter;
	int leader;
	int data;
	int wait;
	int trailer;
} work_t;

typedef struct {
	uint8_t STARTER_HEAD;// = 12;
	uint8_t STARTER_TAIL;// = 115;
	uint8_t LEADER_DAIKIN_HEAD;// = 16;
	uint8_t LEADER_DAIKIN_TAIL;// = 8;

	uint8_t TRAILER_DAIKIN_HEAD;
    uint8_t TRAILER_DAIKIN_TAIL;

	uint8_t WAIT;// = 158; // 79.05ms
} tus_t;

typedef struct {
	IR_Format format;
	int bitlength;
	uint8_t buffer[64];
} data_t;

void tick(void);
void init_timer0();
void init_ticker(void (*)(void));
void init_ir(void);
uint16_t setData(IR_Format  , uint8_t*, uint16_t);

work_t work;
tus_t tus;
data_t data;

#endif
