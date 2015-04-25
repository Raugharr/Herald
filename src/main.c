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
#include "sys/TaskPool.h"
#include "AI/BehaviorTree.h"
#include "AI/Setup.h"

#include <stdlib.h>
#ifdef _WIN32
	#include <io.h>
#else
	#include <sys/io.h>
#endif
#include <lua/lauxlib.h>
#include <lua/lualib.h>

int main(int argc, char* args[]) {
	g_Log.Level = ELOG_ALL;
	LogSetFile("Log.txt");
 	HeraldInit();
 	VideoInit();
	WorldInit(300);

	while(g_VideoOk != 0) {
		Events();
		Draw();
	}

	HeraldDestroy();
	VideoQuit();
	WorldQuit();
	return 0;
}
