/*
 * File: Random.c
 * Author: David Brotz
 */

#include "Random.h"

#include <stdlib.h>

 static unsigned int g_Seed = 23435363;
 static int g_Number = 3446;

 int Rand() {
	g_Number += g_Seed;
	return g_Number;
 }

 unsigned int Random(unsigned int _Min, unsigned int _Max) {
	return abs(Rand() % _Max + _Min);
}

 void Srand(int _Seed) {
	g_Seed = _Seed;
	g_Number = g_Seed >> 2;
}


