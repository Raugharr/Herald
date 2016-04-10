#include "Coroutine.h"

#include "LuaCore.h"

#include <string.h>
#include <alloca.h>
#include <stdarg.h>
#include <SDL2/SDL.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

#define COSCH_MAX 32

struct Coroutine g_Coroutines[COSCH_MAX];
jmp_buf g_CoScheduleJmp;
void* g_ScheduleStack;
int g_CoRun = 0;
int g_CoId = 0;
int g_CoThread = -1; //luaL_ref of thread table.
/*
struct CoScheduler {
	struct Coroutine Coroutines[CO_MAX];
	jmp_buf Jmp;
	void* Stack;
	int CoRun;
	int CoId;
};*/

//struct CoScheduler g_CoSchedulers[COSCH_MAX];

void CoSchedule(Coroutine _Routine) {
	int _Status = 0;
	
	g_ScheduleStack = __builtin_frame_address(0) - CO_STACKSZ;
	for(int i = 0; i < COSCH_MAX; ++i)
		g_Coroutines[i].Status = CO_DEAD;
	lua_newtable(g_LuaState);
	g_CoThread = luaL_ref(g_LuaState, LUA_REGISTRYINDEX);
	g_CoRun = CoSpawn(_Routine, 0);
	g_Coroutines[g_CoRun].LState = g_LuaState;
	_Status = setjmp(g_CoScheduleJmp);
	if(_Status == 0) {
		status:
		;
		volatile char* _Buf = alloca(CO_STACKSZ);

		g_Coroutines[g_CoRun].Status = CO_NORMAL;
		if(g_Coroutines[g_CoRun].ArgCt != 0) {
			__asm__("movl %0, %%eax\n\t"
					"loop_start:\n\t"
					"cmp $0, %%eax\n\t"
					"jle loop_end\n\t"
					"movl -4(%1, %%eax, 4), %%edx\n\t"
					"pushl %%edx\n\t"
					"sub $1, %%eax\n\t"
					"jmp loop_start\n\t"
					"loop_end:\n\t"
					"call *%2\n\t"
					:
					: "r" (g_Coroutines[g_CoRun].ArgCt), "r" (g_Coroutines[g_CoRun].Args), "m" (g_Coroutines[g_CoRun].Routine)
					: "eax", "edx"
			);
		} else {
			g_Coroutines[g_CoRun].Routine();
		}
		g_Coroutines[g_CoRun].Status = CO_DEAD;
		if(g_Coroutines[g_CoRun].From != NULL) {
			struct Coroutine* _Coro = g_Coroutines[g_CoRun].From;
			volatile int8_t* _Stack = alloca(CO_STACKSZ);

			g_CoRun = _Coro->Id;
			memcpy(g_ScheduleStack - _Coro->StackSz, _Coro->Stack, _Coro->StackSz);
			longjmp(_Coro->Jmp, 1);
		}
	} else {
		int _StackSz = 0;
		
		--_Status;
		_StackSz = g_Coroutines[_Status].StackSz;
		g_CoRun = _Status;
		if(g_Coroutines[_Status].Status == CO_DEAD)
			goto status;
		memcpy(g_ScheduleStack - _StackSz, g_Coroutines[_Status].Stack, _StackSz);
		longjmp(g_Coroutines[_Status].Jmp, 1);
	}
}

int CoSpawn(Coroutine _Routine, int _Argc, ... ) {
	va_list _VList;
	int _SkipCt = 0;
	int _StartId = g_CoId;

	while(g_Coroutines[g_CoId].Status != CO_DEAD && g_CoId < COSCH_MAX) {
		++g_CoId;
		++_SkipCt;
	}
	if(g_CoId >= COSCH_MAX) {
		g_CoId = 0;
		while(g_Coroutines[g_CoId].Status != CO_DEAD && g_CoId < _StartId) {
			++g_CoId;
			++_SkipCt;
		}
		if(_SkipCt == COSCH_MAX)
			return -1;
		SDL_assert(g_Coroutines[g_CoId].Status == CO_DEAD);
	}
	va_start(_VList, _Argc);
	SDL_assert(g_CoId < COSCH_MAX);
	g_Coroutines[g_CoId].Stack[0] = 0;
	g_Coroutines[g_CoId].StackSz = 0;
	g_Coroutines[g_CoId].Routine = _Routine;
	g_Coroutines[g_CoId].Status = CO_DEAD;
	g_Coroutines[g_CoId].Routine = _Routine;
	g_Coroutines[g_CoId].Id = g_CoId;
	if(g_CoId == 0)
		g_Coroutines[0].From = NULL;
	else
		g_Coroutines[g_CoId].From = &g_Coroutines[g_CoRun];

	g_Coroutines[g_CoId].ArgCt = _Argc;
	if(g_Coroutines[g_CoId].Args == NULL)
		g_Coroutines[g_CoId].Args = (void*) calloc(_Argc, sizeof(void*));
	for(int i = 0; i < _Argc; ++i)
		g_Coroutines[g_CoId].Args[i] = va_arg(_VList, void*);
	va_end(_VList);
	++g_CoId;
	return g_CoId - 1;
}

void CoResume(int _Coroutine) {
	void* _Rbp = __builtin_frame_address(0);
	void* _OffRbp = (char*) _Rbp - 0x100; //-0x100 for local variables and stack usage.
	int _StackSz = (char*)g_ScheduleStack - (char*)_OffRbp;

	SDL_assert(_StackSz < CO_STACKSZ);
	g_Coroutines[g_CoRun].StackSz = _StackSz;
	g_Coroutines[g_CoRun].Status = CO_SUSPENDED;
	if(g_Coroutines[_Coroutine].LState == NULL) {
		lua_State* _NewState = NULL;

		lua_rawgeti(g_LuaState, LUA_REGISTRYINDEX, g_CoThread);
		lua_pushinteger(g_LuaState, lua_rawlen(g_LuaState, -1) + 1);
		_NewState = lua_newthread(g_LuaState);
		g_Coroutines[_Coroutine].LState = _NewState;
		lua_rawset(g_LuaState, -3);
		lua_pop(g_LuaState, 1);
	}
	g_LuaState = g_Coroutines[_Coroutine].LState;
	memcpy(g_Coroutines[g_CoRun].Stack, _OffRbp, _StackSz);
	if(setjmp(g_Coroutines[g_CoRun].Jmp) != 0)
		return;
	longjmp(g_CoScheduleJmp, _Coroutine + 1);
}

void CoYield() {
	struct Coroutine* _From = g_Coroutines[g_CoRun].From;
	void* _Rbp = __builtin_frame_address(0);
	void* _OffRbp = (char*) _Rbp - 0x100; //-0x100 for local variables and stack usage.
	int _StackSz = (char*)g_ScheduleStack - (char*)_OffRbp;

	_From->Status = CO_NORMAL;
	g_Coroutines[g_CoRun].StackSz = _StackSz;
	g_Coroutines[g_CoRun].Status = CO_SUSPENDED;
	memcpy(g_Coroutines[g_CoRun].Stack, _OffRbp, _StackSz);
	if(setjmp(g_Coroutines[g_CoRun].Jmp) != 0)
		return;
	longjmp(g_CoScheduleJmp, _From->Id + 1);	
}

int CoStackRem() {
	return CO_STACKSZ - (__builtin_frame_address(0) - g_ScheduleStack);
}

int CoRunning() {
	return g_CoRun;
}
//PID: 1
//CoSpawn(2)
//CoResume(2)
//
//PID: 2
//CoYield(1)
/*
	CoSchedule(GameStuff)

	GameStuff() {
		CoSpawn(Event);
		while(1) {
		...
		CoYield(2)
		};

	}

	Event {
		while(1) {
			...
			CoYield(0)
		}
	}
*/
