/*
 * Author: David Brotz
 * File: Event.c
 */

#include "Event.h"

#include "../sys/MemoryPool.h"
#include "../sys/LinkedList.h"
#include "../sys/Queue.h"

#include <stdlib.h>

#define EVENTPOOL (1024)

static struct Queue* g_EventQueue = NULL;
static struct LinkedList** g_EventHooks = NULL;
static struct MemoryPool* g_MemoryPool = NULL;

void EventInit() {
	int i;

	g_EventQueue = CreateQueue(EVENTPOOL);
	g_EventHooks = (struct LinkedList**) malloc(sizeof(struct LinkedList**) * EVENTLAST);
	g_MemoryPool = CreateMemoryPool(sizeof(struct Event), EVENTPOOL);
	for(i = 0; i < EVENTLAST; ++i)
		g_EventHooks[i] = CreateLinkedList();
}

void EventQuit() {
	int i;

	DestroyQueue(g_EventQueue);
	DestroyMemoryPool(g_MemoryPool);
	for(i = 0; i < EVENTLAST; ++i)
		DestroyLinkedList(g_EventHooks[i]);
	free(g_EventHooks);
}

void EventPush(struct Event* _Event) {
	Queue_Push(g_EventQueue, _Event);
}

void EventHook(int _EventId, void (*_Callback)(struct Event*)) {
	if(_EventId < 0 && _EventId >= EVENTLAST)
		return;
	LnkLst_PushBack(g_EventHooks[_EventId], &_Callback);
}

void EventRmHook(int _EventId, void(*_Callback)(struct Event*)) {
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

struct Event* CreateEventBirth(struct Person* _Mother, struct Person* _Child) {
	struct EventBirth* _EventBirth = (struct EventBirth*) malloc(sizeof(struct EventBirth));

	_EventBirth->Type = EVENTBIRTH;
	_EventBirth->Mother = _Mother;
	_EventBirth->Child = _Child;
	return ((struct Event*)_EventBirth);
}

struct Event* CreateEventDeath(struct Person* _Person) {
	struct EventDeath* _EventDeath = (struct EventDeath*) malloc(sizeof(struct EventDeath));

	_EventDeath->Type = EVENTDEATH;
	_EventDeath->Person = _Person;
	return ((struct Event*)_EventDeath);
}

struct Event* CreateEventTime(struct Person* _Person, DATE _Age) {
	struct EventAge* _EventAge = (struct EventAge*) malloc(sizeof(struct EventAge));

	_EventAge->Type = EVENTAGE;
	_EventAge->Person = _Person;
	_EventAge->Age = _Age;
	return ((struct Event*)_EventAge);
}
