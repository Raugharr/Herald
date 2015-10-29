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
#include "video/VideoLua.h"

#include "sys/Event.h"
#include "sys/Log.h"
#include "sys/HashTable.h"
#include "sys/LuaCore.h"
#include "sys/TaskPool.h"
#include "sys/ResourceManager.h"

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
		{"GetImprovingRelation", NULL},
		{"SetMeanTime", LuaMissionSetMeanTime},
		{"SetId", LuaMissionSetId},
		{"OnTrigger", LuaMissionOnTrigger},
		{"CallById", LuaMissionCallById},
		{NULL, NULL},
};

int InitLuaSystem() {
	InitLuaCore();
	InitLuaFamily();
	RegisterLuaObjects(g_LuaState, g_LuaSettlementObjects);
	RegisterLuaObjects(g_LuaState, g_LuaAIObjects);

	lua_settop(g_LuaState, 0);
	luaL_newlib(g_LuaState, g_LuaFuncsMission);
	luaL_getmetatable(g_LuaState, "BigGuy");
	lua_createtable(g_LuaState, 0, 0);
	LuaCopyTable(g_LuaState, -2);
	lua_setmetatable(g_LuaState, -2);

	lua_pushstring(g_LuaState, "LessThan");
	lua_pushinteger(g_LuaState, WSOP_LESSTHAN);
	lua_rawset(g_LuaState, -3);

	lua_pushstring(g_LuaState, "GreaterThan");
	lua_pushinteger(g_LuaState, WSOP_GREATERTHAN);
	lua_rawset(g_LuaState, -3);

	lua_pushstring(g_LuaState, "Equal");
	lua_pushinteger(g_LuaState, WSOP_EQUAL);
	lua_rawset(g_LuaState, -3);
	lua_setglobal(g_LuaState, "Mission");

	InitVideoLua(g_LuaState);
	lua_newtable(g_LuaState);
	lua_pushstring(g_LuaState, "__index");
	lua_pushvalue(g_LuaState, LUA_REGISTRYINDEX);
	lua_pushstring(g_LuaState, "Animation");
	lua_rawget(g_LuaState, -2);
	lua_pop(g_LuaState, 1);
	lua_rawset(g_LuaState, -3);
	lua_setmetatable(g_LuaState, -2);
	LuaSetEnv(g_LuaState, "Animation");
	lua_pop(g_LuaState, 1);

	LuaWorldInit();
	Log(ELOG_INFO, "Loading Missions");
	++g_Log.Indents;
	LoadAllMissions(g_LuaState, &g_MissionEngine);
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
	struct PakFile _Pak;
	struct System _Systems[] = {
			{"Main", HeraldInit, HeraldDestroy},
			{"Lua", InitLuaSystem, QuitLuaCore},
			{"Video", VideoInit, VideoQuit},
			{"Reform", InitReforms, QuitReforms},
			{NULL, NULL, NULL}
	};
	g_Log.Level = ELOG_ALL;
	LogSetFile("Log.txt");
#ifdef DEBUG
	chdir("data");
	CreatePak("graphics");
	chdir("..");
#endif
	ResourceManagementInit(1);
	RsrMgrConstruct(&_Pak, "data/graphics.pak");
	for(_SysCt = 0; _Systems[_SysCt].Name != NULL; ++_SysCt) {
		Log(ELOG_INFO, "Initializing %s system.", _Systems[_SysCt].Name);
		if(_Systems[_SysCt].Init() == 0) {
			Log(ELOG_INFO, "System %s could not be loaded.", _Systems[_SysCt].Name);
			goto quit;
		}
	}
	AnimationLoad(g_LuaState, "data/anim/test.lua");
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
	quit:
	//DestroyPak(&_Pak);
	ResourceManagementQuit();
	WorldQuit();
	IMG_Quit();
	for(_SysCt = _SysCt - 1;_SysCt >= 0; --_SysCt) {
		Log(ELOG_INFO, "Quitting %s system.", _Systems[_SysCt].Name);
		_Systems[_SysCt].Quit();
	}
	return 0;
}
