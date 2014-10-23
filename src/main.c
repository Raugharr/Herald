/*
 * Author: David Brotz
 * File: Main.c
 */

#include "Herald.h"
#include "World.h"
#include "Person.h"
#include "sys/Log.h"
#include "sys/Video.h"
#include "sys/HashTable.h"
#include "sys/LuaHelper.h"
#include "AI/BehaviorTree.h"
#include "AI/Setup.h"

#include <stdlib.h>
#ifdef WINDOWS
	#include <io.h>
#else
	#include <sys/io.h>
#endif
#include <lua/lauxlib.h>
#include <lua/lualib.h>

struct HashTable* g_AIHash = NULL;

int Tick() {
	struct Person* _Person = g_PersonList;
	int _Ct = 0;
	int _Size = 0;

	ATImerUpdate(&g_ATimer);
	while(_Person != NULL) {
		if(_Person->Age < TO_YEARS(13))
			_Ct++;
		else
			++_Size;
		HashClear(g_AIHash);
		BHVRun(_Person->Behavior, _Person, g_AIHash);
		_Person = _Person->Next;
	}
	if(World_Tick() == 0)
		return 0;
	return 1;
}

int main(int argc, char* args[]) {
	g_AIHash = CreateHash(32);
	LogSetFile("Log.txt");
	g_LuaState = luaL_newstate();
	luaL_openlibs(g_LuaState);
	RegisterLuaFuncs(g_LuaState);
	atexit(LogCloseFile);
 	HeraldInit();
 	VideoInit();
	WorldInit(300);

	g_AIHash = CreateHash(32);
	//for(i = 0; i < 366; ++i)
	//	Tick();
	while(g_GUIOk != 0) {
		Events();
		Draw();
	}

	HeraldDestroy();
	VideoQuit();
	WorldQuit();
	DestroyHash(g_AIHash);
	return 0;
}
