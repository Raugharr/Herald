/*
 * File: Random.c
 * Author: David Brotz
 */

#include "Random.h"

#include <stdlib.h>

 static unsigned int g_SeedX = 521288629;
 static unsigned int g_SeedY = 362436069;

 int Rand() {
	g_SeedX = 1800 * (g_SeedX & 65535) + (g_SeedX >> 16);
	g_SeedY = 30903 * (g_SeedY & 65535) + (g_SeedY >> 16);
	return (g_SeedX << 16) + (g_SeedY & 65535);
 }

 unsigned int Random(unsigned int _Min, unsigned int _Max) {
	return Rand() % (_Max - _Min + 1) + _Min;
}

 void Srand(int _Seed) {
	g_SeedX = _Seed << 16;
	g_SeedY = _Seed >> 16;
}


