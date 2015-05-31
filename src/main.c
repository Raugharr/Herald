/*
 * Author: David Brotz
 * File: Main.c
 */

#include "Herald.h"
#include "World.h"
#include "Person.h"
#include "Government.h"
#include "LuaFamily.h"
#include "LuaSettlement.h"
#include "LuaWorld.h"

#include "video/Video.h"

#include "sys/Log.h"
#include "sys/HashTable.h"
#include "sys/LuaCore.h"
#include "sys/TaskPool.h"

#include "AI/BehaviorTree.h"
#include "AI/Setup.h"

#include <SDL2/SDL_image.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

#include <stdlib.h>
#ifdef _WIN32
	#include <io.h>
#else
	#include <sys/io.h>
#endif

void InitLuaSystem() {
	InitLuaCore();
	InitLuaFamily();
	RegisterLuaObjects(g_LuaState, g_LuaSettlementObjects);
	LuaWorldInit();
}

int main(int argc, char* args[]) {
	int i = 0;
	int _WorldTimer = 0;
	int _DrawTimer = 0;
	int _Ticks = 0;
	struct System _Systems[] = {
			{"Lua", InitLuaSystem, QuitLuaCore},
			{"Reform", InitReforms, QuitReforms},
			{"Main", HeraldInit, HeraldDestroy},
			{"Video", VideoInit, VideoQuit},
			{NULL, NULL, NULL}
	};
	g_Log.Level = ELOG_ALL;
	LogSetFile("Log.txt");
	g_LuaState = luaL_newstate();
	for(i = 0; _Systems[i].Name != NULL; ++i) {
		Log(ELOG_INFO, "Initializing %s system.", _Systems[i].Name);
		_Systems[i].Init();
	};
	IMG_Init(IMG_INIT_PNG);
	WorldInit(300);


	_WorldTimer = 0;
	while(g_VideoOk != 0) {
		_Ticks = SDL_GetTicks();
		Events();
		if(_DrawTimer + 16 <= _Ticks) {
			Draw();
			_DrawTimer = _Ticks;
		}
		if(g_GameWorld.IsPaused == 0 && (_WorldTimer + 1000) <= _Ticks) {
			World_Tick();
			_WorldTimer = _Ticks;
		}
		++g_TaskPool->Time;
	}
	IMG_Quit();
	WorldQuit();
	for(i = 0; _Systems[i].Name != NULL; ++i) {
		Log(ELOG_INFO, "Quitting %s system.", _Systems[i].Name);
		_Systems[i].Quit();
	}
	lua_close(g_LuaState);
	return 0;
}
