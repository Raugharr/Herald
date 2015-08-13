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

#include "video/GuiLua.h"
#include "video/Video.h"

#include "sys/Log.h"
#include "sys/HashTable.h"
#include "sys/LuaCore.h"
#include "sys/TaskPool.h"

#include "AI/BehaviorTree.h"
#include "AI/Setup.h"
#include "AI/LuaAi.h"

#include <SDL2/SDL_image.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

#include <stdlib.h>
#ifdef _WIN32
	#include <io.h>
#else
	#include <sys/io.h>
#endif

int InitLuaSystem() {
	InitLuaCore();
	InitLuaFamily();
	RegisterLuaObjects(g_LuaState, g_LuaSettlementObjects);
	RegisterLuaObjects(g_LuaState, g_LuaAIObjects);
	LuaWorldInit();
		WorldInit(300);
	if(InitGUILua(g_LuaState) == 0)
		return 0;
	return 1;
}

void QuitLuaSystem() {
	QuitGUILua(g_LuaState);
	QuitLuaCore();
	WorldQuit();
	lua_close(g_LuaState);
}

int main(int argc, char* args[]) {
	int i = 0;
	int _WorldTimer = 0;
	int _DrawTimer = 0;
	int _Ticks = 0;
	struct System _Systems[] = {
			{"Main", HeraldInit, HeraldDestroy},
			{"Video", VideoInit, VideoQuit},
			{"Reform", InitReforms, QuitReforms},
			{"Lua", InitLuaSystem, QuitLuaCore},
			{NULL, NULL, NULL}
	};
	g_Log.Level = ELOG_ALL;
	LogSetFile("Log.txt");
	g_LuaState = luaL_newstate();
	for(i = 0; _Systems[i].Name != NULL; ++i) {
		Log(ELOG_INFO, "Initializing %s system.", _Systems[i].Name);
		if(_Systems[i].Init() == 0) {
			Log(ELOG_INFO, "System %s could not be loaded.", _Systems[i].Name);
			goto quit;
		}
	};
	IMG_Init(IMG_INIT_PNG);

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
	quit:
	IMG_Quit();
	for(i = 0; _Systems[i].Name != NULL; ++i) {
		Log(ELOG_INFO, "Quitting %s system.", _Systems[i].Name);
		_Systems[i].Quit();
	}
	return 0;
}
