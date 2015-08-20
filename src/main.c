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
#include "Mission.h"

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

static const luaL_Reg g_LuaFuncsMission[] = {
		{"SetName", LuaMissionSetName},
		{"SetDesc", LuaMissionSetDesc},
		{"AddOption", LuaMissionAddOption},
		{"AddTrigger", LuaMissionAddTrigger},
		{"Owner", LuaMissionGetOwner},
		{"GetRandomPerson", LuaMissionGetRandomPerson},
		{NULL, NULL},
};

int InitLuaSystem() {
	InitLuaCore();
	InitLuaFamily();
	RegisterLuaObjects(g_LuaState, g_LuaSettlementObjects);
	RegisterLuaObjects(g_LuaState, g_LuaAIObjects);

	luaL_newlib(g_LuaState, g_LuaFuncsMission);
	lua_createtable(g_LuaState, 0, 0);
	luaL_getmetatable(g_LuaState, "BigGuy");
	LuaCopyTable(g_LuaState, -2);
	//lua_pushstring(g_LuaState, "__call");
	//lua_pushcfunction(g_LuaState, LuaMissionCall);
	//lua_rawset(g_LuaState, -3);
	lua_setmetatable(g_LuaState, -2);
	lua_pushstring(g_LuaState, "LessThan");
	lua_pushinteger(g_LuaState, WSOP_LESSTHAN);
	lua_rawset(g_LuaState, -3);
	lua_setglobal(g_LuaState, "Mission");

	LuaWorldInit();
	Log(ELOG_INFO, "Loading Missions");
	++g_Log.Indents;
	LoadAllMissions(g_LuaState, &g_MissionList);
	--g_Log.Indents;
	return 1;
}

void QuitLuaSystem() {
	QuitLuaCore();
}

int main(int argc, char* args[]) {
	int _WorldTimer = 0;
	int _DrawTimer = 0;
	int _Ticks = 0;
	int _SysCt = 0;
	struct System _Systems[] = {
			{"Lua", InitLuaSystem, QuitLuaCore},
			{"Main", HeraldInit, HeraldDestroy},
			{"Video", VideoInit, VideoQuit},
			{"Reform", InitReforms, QuitReforms},
			{NULL, NULL, NULL}
	};
	g_Log.Level = ELOG_ALL;
	LogSetFile("Log.txt");
	for(_SysCt = 0; _Systems[_SysCt].Name != NULL; ++_SysCt) {
		Log(ELOG_INFO, "Initializing %s system.", _Systems[_SysCt].Name);
		if(_Systems[_SysCt].Init() == 0) {
			Log(ELOG_INFO, "System %s could not be loaded.", _Systems[_SysCt].Name);
			goto quit;
		}
	}
	WorldInit(300);
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
	WorldQuit();
	IMG_Quit();
	for(_SysCt = _SysCt - 1;_SysCt >= 0; --_SysCt) {
		Log(ELOG_INFO, "Quitting %s system.", _Systems[_SysCt].Name);
		_Systems[_SysCt].Quit();
	}
	return 0;
}
