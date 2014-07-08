/*
 * Author: David Brotz
 * File: Main.c
 */
 
#include "Manor.h"
#include "Herald.h"
#include "World.h"
#include "Person.h"
#include "sys/Log.h"
#include "video/Video.h"
#include "sys/HashTable.h"
#include "AI/BehaviorTree.h"
#include "AI/Setup.h"

#include <stdlib.h>

struct HashTable* g_AIHash = NULL;

int Tick() {
	struct Person* _Person = g_PersonList;

	ATImerUpdate(&g_ATimer);
	while(_Person != NULL) {
		HashClear(g_AIHash);
		if(TO_YEARS(_Person->Age) < 13)
			BHVRun(g_AIChild, _Person, g_AIHash);
		else if(_Person->Gender == EMALE)
			BHVRun(g_AIMan, _Person, g_AIHash);
		else
			BHVRun(g_AIWoman, _Person, g_AIHash);
		_Person = _Person->Next;
	}
	if(World_Tick() == 0)
		return 0;
	return 1;
}

int main(int argc, char* args[]) {
	int i;

	g_AIHash = CreateHash(32);
	LogSetFile("Log.txt");

	atexit(LogCloseFile);
 	HeraldInit();
 	AIInit();
	World_Init(300);

	g_AIHash = CreateHash(32);
	for(i = 0; i < 366; ++i)
		Tick();

	World_Quit();
	AIQuit();
	HeraldDestroy();
	DestroyHash(g_AIHash);
	return 0;
}
