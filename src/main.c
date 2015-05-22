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

#include "video/Video.h"

#include "sys/Log.h"
#include "sys/HashTable.h"
#include "sys/LuaCore.h"
#include "sys/TaskPool.h"

#include "AI/BehaviorTree.h"
#include "AI/Setup.h"

#include <SDL2/SDL_image.h>

#include <stdlib.h>
#ifdef _WIN32
	#include <io.h>
#else
	#include <sys/io.h>
#endif

int main(int argc, char* args[]) {
	int i = 0;
	struct System _Systems[] = {
			{"Lua", InitLuaSystem, QuitLuaSystem},
			{"Reform", InitReforms, QuitReforms},
			{"Main", HeraldInit, HeraldDestroy},
			{"Video", VideoInit, VideoQuit},
			{NULL, NULL, NULL}
	};
	g_Log.Level = ELOG_ALL;
	LogSetFile("Log.txt");
	for(i = 0; _Systems[i].Name != NULL; ++i) {
		Log(ELOG_INFO, "Initializing %s system.", _Systems[i].Name);
		_Systems[i].Init();
	};
	RegisterLuaObjects(g_LuaState, g_LuaFamilyObjects);
	RegisterLuaObjects(g_LuaState, g_LuaSettlementObjects);
	IMG_Init(IMG_INIT_PNG);
	WorldInit(300);

	while(g_VideoOk != 0) {
		Events();
		Draw();
	}
	IMG_Quit();
	WorldQuit();
	for(i = 0; _Systems[i].Name != NULL; ++i) {
		Log(ELOG_INFO, "Quitting %s system.", _Systems[i].Name);
		_Systems[i].Quit();
	}
	return 0;
}
