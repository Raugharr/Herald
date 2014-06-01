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
	HeraldInit();
	World_Init(300);
	Tick();
	World_Quit();
	HeraldDestroy();
	return 0;
}
