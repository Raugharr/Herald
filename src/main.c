/*
 * Author: David Brotz
 * File: Main.c
 */
 
#include "Manor.h"
#include "Herald.h"
#include "World.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

int main(int argv, char** argc) {
	int i;
 	HeraldInit();
	World_Init(300);
	for(i = 0; i < 366; ++i)
		Tick();
	World_Quit();
	HeraldDestroy();
	return 0;
}
