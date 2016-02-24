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
	struct Coroutine* From;
	int8_t Status;
	int8_t Stack[CO_STACKSZ];
	int StackSz; //How much of the stack has been used.
	int Id;
	void** Args;
	int ArgCt;
	lua_State* LState;
	Coroutine Routine;
};

void CoSchedule(Coroutine _Routine);
int CoSpawn(Coroutine _Routine, int _Argc, ...);
void CoYield();
void CoResume(int _Coroutine);
int CoStackRem();
int CoRunning();
#endif
