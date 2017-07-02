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
#include "LuaGovernment.h"
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
#include "sys/FrameAllocator.h"

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
			FrameFree();
		}
		++g_TaskPool->Time;
		lua_settop(g_LuaState, 0);
	}
}

int main(int argc, char* args[]) {
	int _SysCt = 0;
	struct PakFile _Pak;
	struct System _Systems[] = {
			{"Main", HeraldInit, HeraldDestroy},
			{"Lua", InitLuaSystem, QuitLuaSystem},
			{"Video", VideoInit, VideoQuit},
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
	AIInit(g_LuaState);
	AnimationLoad(g_LuaState, "data/anim/test.lua");
	WorldInit(150, 1);

	CoSchedule(MainCoro);
	quit:
	ResourceManagementQuit();
	WorldQuit();
	AIQuit();
	IMG_Quit();
	for(_SysCt = _SysCt - 1;_SysCt >= 0; --_SysCt) {
		Log(ELOG_INFO, "Quitting %s system.", _Systems[_SysCt].Name);
		_Systems[_SysCt].Quit();
	}
	return 0;
}
