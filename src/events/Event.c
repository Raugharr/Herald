/*
 * Author: David Brotz
 * File: Event.c
 */

#include "Event.h"

#include "../sys/MemoryPool.h"
#include "../sys/LinkedList.h"
#include "../sys/Queue.h"

#include <stdlib.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

#define EVENTPOOL (1024)

static struct Queue* g_EventQueue = NULL;
static struct LinkedList** g_EventHooks = NULL;
static struct MemoryPool* g_MemoryPool = NULL;

void Event_Init() {
	int i;

	g_EventQueue = CreateQueue(EVENTPOOL);
	g_EventHooks = (struct LinkedList**) malloc(sizeof(struct LinkedList**) * EVENTLAST);
	g_MemoryPool = CreateMemoryPool(sizeof(struct Event), EVENTPOOL);
	for(i = 0; i < EVENTLAST; ++i)
		g_EventHooks[i] = CreateLinkedList();
}

void Event_Quit() {
	int i;

	DestroyQueue(g_EventQueue);
	DestroyMemoryPool(g_MemoryPool);
	for(i = 0; i < EVENTLAST; ++i)
		DestroyLinkedList(g_EventHooks[i]);
	free(g_EventHooks);
}

void Event_Push(struct Event* _Event) {
	Queue_Push(g_EventQueue, _Event);
}

void Event_Hook(int _EventId, void (*_Callback)(struct Event*)) {
	if(_EventId < 0 && _EventId >= EVENTLAST)
		return;
	LnkLst_PushBack(g_EventHooks[_EventId], &_Callback);
}

void Event_RemoveHook(int _EventId, void(*_Callback)(struct Event*)) {
	struct LnkLst_Node* _Last = NULL;
	struct LnkLst_Node* _Node = NULL;
	struct LinkedList* _List = NULL;

	if(_EventId < 0 && _EventId >= EVENTLAST)
		return;
	_List = g_EventHooks[_EventId];
	_Node = _List->Front;
	if(*((void(*)(struct Event*))(_List->Front->Data)) == _Callback) {
		LnkLst_PopFront(_List);
		return;
	}
	_Last = _Node;
	_Node = _Node->Next;

	while(_Node != NULL) {
		if(*((void(*)(struct Event*))(_Node->Data)) == _Callback) {
			LnkLst_Remove(_List, _Last, _Node);
			return;
		}
		_Last = _Node;
		_Node = _Node->Next;
	}
}
