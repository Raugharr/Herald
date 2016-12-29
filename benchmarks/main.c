/*
 * Author: David Brotz
 * File: Main.c
 */

#include "../src/sys/Coroutine.h"
#include "../src/sys/LuaCore.h"

#include <SDL2/SDL.h>

void CoroBench(int One, int Two) {
	for(int i = 0; i < 1000000; ++i) {
	//	int Three = One + Two;
		CoYield();
	}
}

void RegBench(int One, int Two) {
	for(int i = 0; i < 1000000; ++i) {
		int Three = One + Two;
	}
}

void MainCoro() {
	int Id = CoSpawn(CoroBench, 2, 1, 8);

	CoResume(Id);
	while(CoStatus(Id) != CO_DEAD)
		CoResume(Id);
}

int main(int argc, const char* argv[]) {
	InitLuaCore();

	int Start = SDL_GetTicks();

	CoSchedule(MainCoro);
	printf("Coroutine time: %i\n", SDL_GetTicks() - Start);

	Start = SDL_GetTicks();
	RegBench(1, 8);
	printf("Loop time: %i\n", SDL_GetTicks() - Start);
	return 0;
}
