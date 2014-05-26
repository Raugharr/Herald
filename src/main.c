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
	struct Manor* _Manor = NULL;

	HeraldInit();
	_Manor = CreateManor("Test", 500);
	DestroyManor(_Manor);
	HeraldDestroy();
	return 0;
}
