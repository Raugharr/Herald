/*
 * Author: David Brotz
 * File: Main.c
 */
 
#include "Manor.h"
#include "Herald.h"
#include "World.h"
#include "Person.h"
#include "sys/HashTable.h"
#include "AI/BehaviorTree.h"
#include "AI/Setup.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

struct HashTable* g_AIHash = NULL;

int Tick() {
	struct Person* _Person = g_PersonList;

	ATImerUpdate(&g_ATimer);
	while(_Person != NULL) {
		HashClear(g_AIHash);
		if(TO_YEARS(_Person->Age) < 13)
			BHVRun(g_AIChild, _Person, NULL);
		else if(_Person->Gender == EMALE)
			BHVRun(g_AIMan, _Person, NULL);
		else
			BHVRun(g_AIWoman, _Person, NULL);
		_Person = _Person->Next;
	}
	if(World_Tick() == 0)
		return 0;
	return 1;
}

int main(int argv, char** argc) {
	int i;

	g_AIHash = CreateHash(32);
 	HeraldInit();
 	AIInit();
	World_Init(300);
	for(i = 0; i < 366; ++i)
		Tick();
	World_Quit();
	AIQuit();
	HeraldDestroy();
	DestroyHash(g_AIHash);
	return 0;
}
