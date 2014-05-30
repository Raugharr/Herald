/*
 * Author: David Brotz
 * File: Event.c
 */

#include "Event.h"

#include "../sys/MemoryPool.h"
#include "../sys/Queue.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

#define EVENTPOOL (1024)

static struct Queue* g_EventQueue = NULL;
static struct MemoryPool* g_MemoryPool = NULL;

void Event_Init() {
	g_EventQueue = CreateQueue(EVENTPOOL);
	g_MemoryPool = CreateMemoryPool(sizeof(struct Event), EVENTPOOL);
}

void Event_Quit() {
	DestroyQueue(g_EventQueue);
	DestroyMemoryPool(g_MemoryPool);
}

void Event_Push(struct Event* _Event) {Queue_Push(g_EventQueue, _Event);}
