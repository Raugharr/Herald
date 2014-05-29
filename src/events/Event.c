/*
 * Author: David Brotz
 * File: Event.c
 */

//#include "Event.h"

//#include "../sys/MemoryPool.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

//static struct MemoryPool* g_MemoryPool = NULL;

void Event_Init() {
//	g_MemoryPool = CreateMemoryPool(sizeof(struct Event), 1024);
}

void Event_Quit() {
//	DestroyMemoryPool(g_MemoryPool);
}
