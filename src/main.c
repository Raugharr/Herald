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
#include "BigGuy.h"

#include "video/GuiLua.h"
#include "video/Video.h"
#include "video/VideoLua.h"

#include "sys/Event.h"
#include "sys/Log.h"
#include "sys/HashTable.h"
#include "sys/LuaCore.h"
#include "sys/TaskPool.h"
#include "sys/ResourceManager.h"

#include "sys/Coroutine.h"

#include "AI/BehaviorTree.h"
#include "AI/Setup.h"
#include "AI/LuaAI.h"

#include <SDL2/SDL_image.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <unistd.h>

#include <stdlib.h>
#ifdef _WIN32
	#include <io.h>
#else
	#include <sys/io.h>
#endif

#define GAME_TICK (200)

void LuaSettlementObjects(lua_State* _State) {
	lua_newtable(_State);
	lua_pushstring(_State, "Relation");
	lua_newtable(_State);

	lua_pushstring(_State, "Token");
	lua_pushinteger(_State, OPINION_TOKEN);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Small");
	lua_pushinteger(_State, OPINION_SMALL);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Average");
	lua_pushinteger(_State, OPINION_AVERAGE);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Great");
	lua_pushinteger(_State, OPINION_GREAT);
	lua_rawset(_State, -3);

	lua_rawset(_State, -3);

	lua_pushstring(_State, "Action");
	lua_newtable(_State);

	lua_pushstring(_State, "Influence");
	lua_pushinteger(_State, BGACT_IMRPOVEREL);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Sabotage");
	lua_pushinteger(_State, BGACT_SABREL);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Duel");
	lua_pushinteger(_State, BGACT_DUEL);
	lua_rawset(_State, -3);

	lua_rawset(_State, -3);
	lua_setglobal(_State, "BigGuy");
}

int InitLuaSystem() {
	InitLuaCore();
	InitLuaFamily();
	RegisterLuaObjects(g_LuaState, g_LuaSettlementObjects);
	LuaSettlementObjects(g_LuaState);
	RegisterLuaObjects(g_LuaState, g_LuaAIObjects);

	InitMissionLua(g_LuaState);
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

void MainCoro() {
	int _WorldTimer = 0;
	int _DrawTimer = 0;
	int _Ticks = 0;

	while(g_VideoOk != 0) {
		_Ticks = SDL_GetTicks();
		Events();
		if(_DrawTimer + 16 <= _Ticks) {
			Draw();
			_DrawTimer = _Ticks;
		}
		if(g_GameWorld.IsPaused == 0 && (_WorldTimer + GAME_TICK) <= _Ticks) {
			World_Tick();
			_WorldTimer = _Ticks;
		}
		++g_TaskPool->Time;
	}
}

int main(int argc, char* args[]) {
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

	CoSchedule(MainCoro);
	quit:
	ResourceManagementQuit();
	WorldQuit();
	IMG_Quit();
	for(_SysCt = _SysCt - 1;_SysCt >= 0; --_SysCt) {
		Log(ELOG_INFO, "Quitting %s system.", _Systems[_SysCt].Name);
		_Systems[_SysCt].Quit();
	}
	return 0;
}
