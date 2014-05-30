/*
 * Author: David Brotz
 * File: Main.c
 */
 
#include "Manor.h"
#include "Herald.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

int main(int argv, char** argc) {
	HeraldInit();
	HeraldDestroy();
	return 0;
}
