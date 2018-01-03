#ifndef __COROUTINE_H
#define __COROUTINE_H

#include <setjmp.h>
#include <inttypes.h>

#define CO_STACKSZ (0x3500)

typedef void(*Coroutine)();
typedef struct lua_State lua_State;

enum CO_STATUS {
	CO_NONE,
	CO_NORMAL,
	CO_SUSPENDED,
	CO_DEAD,
};

struct Coroutine {
	jmp_buf Jmp; 
	uint32_t Id;
	void** Args;
	int32_t ArgCt;
	uint16_t StackSz; //How much of the stack has been used.
	int16_t From;//Id of coroutine this came from
	int8_t Status;
	lua_State* LState;
	Coroutine Routine;
	int8_t Stack[CO_STACKSZ];
};

void CoSchedule(Coroutine _Routine);
int CoSpawn(Coroutine _Routine, uint8_t _Argc, ...);
void CoYield();
void CoResume(int _Coroutine);
uint8_t CoStatus(int _Coroutine);
uint32_t CoStackRem();
int CoRunning();
#endif
