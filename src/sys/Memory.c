/**
 * File: Memory.c
 * Author: David Brotz
 */

#include "Memory.h"

#include <stdlib.h>

//FIXME: allocate an actual page instead.
void* PageAlloc(size_t _Block) {
	return malloc(_Block);
}

void PageFree(void* _Block) {
	free(_Block);
}

