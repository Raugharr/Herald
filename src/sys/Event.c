/*
 * Author: David Brotz
 * File: Event.c
 */

#include "Event.h"

#include "MemoryPool.h"
#include "LinkedList.h"
#include "Queue.h"
#include "RBTree.h"

#include "../Person.h"
#include "../Family.h"
#include "../Crop.h"

#include <stdlib.h>

#define EVENTPOOL (1024)

static struct EventQueue g_EventQueue = {0, NULL, NULL};
static struct RBTree* g_EventHooks[EVENT_LAST];
int g_EventId = 0;
const char* g_EventNames[] = {
		"Birth",
		"Death",
		"Age",
		"Farming",
		"StarvingFamily",
		NULL
};

int ActorObserverI(const void* _One, const void* _Two) {
	return ((struct EventObserver*)_One)->ObjectId - ((struct EventObserver*)_Two)->ObjectId;
}

int ActorObserverS(const void* _One, const void* _Two) {
	return ((struct EventObserver*)_Two)->ObjectId - *((int*)_Two);
}

struct RBTree g_ActorObservers = {NULL, 0, ActorObserverI, ActorObserverS};

#define EventCtor(_Struct, _Event, _ObjId, _Location, _Type)			\
	(_Event)->Id = g_EventId++;											\
	(_Event)->Location = (_Location);									\
	(_Event)->ObjId = (_ObjId);											\
	(_Event)->Type = (_Type);											\
	(_Event->Next) = NULL

void EventInit() {
	for(int i = 0; i < EVENT_LAST; ++i)
		g_EventHooks[i] = CreateRBTree(ActorObserverI, ActorObserverS);
}

void EventQuit() {
	for(int i = 0; i < EVENT_LAST; ++i)
		DestroyRBTree(g_EventHooks[i]);
}

void EventPush(struct Event* _Event) {
	if(g_EventQueue.Top == NULL) {
		g_EventQueue.Top = _Event;
		g_EventQueue.Bottom = _Event;
	} else {
		g_EventQueue.Bottom->Next = _Event;
		g_EventQueue.Bottom = _Event;
	}
	++g_EventQueue.Size;
}

void EventHook(int _EventType, int _ObjId, void (*_OnEvent)(const void*, void*), void* _Listener) {
	struct EventObserver* _Obs = NULL;
	if(_EventType < 0 || _EventType >= EVENT_LAST)
		return;
	if((_Obs = RBSearch(g_EventHooks[_EventType], &_ObjId)) != NULL) {
		struct EventObserver* _New = CreateEventObserver(_EventType, _ObjId, _OnEvent, _Listener);

		ILL_CREATE(_Obs, _New);
	} else {
		RBInsert(g_EventHooks[_EventType], CreateEventObserver(_EventType, _ObjId, _OnEvent, _Listener));
	}
}

void EventHookRemove(int _EventType, int _ObjId) {
	struct EventObserver* _Obs = NULL;
	struct EventObserver* _Next = NULL;
	struct EventObserver* _Front = NULL;

	if(_EventType < 0 || _EventType >= EVENT_LAST)
		return;
	if((_Obs = RBSearch(g_EventHooks[_EventType], &_ObjId)) != NULL) {
		_Front = _Obs;
		do {
			_Next = _Obs->Next;
			if(_Obs->ObjectId == _ObjId) {
				ILL_DESTROY(_Front, _Obs);
				DestroyEventObserver(_Obs);
			}
			_Obs = _Next;
		} while(_Obs != NULL);
	}
}

void EventHookUpdate(const struct Event* _Event) {
	struct EventObserver* _Obs = NULL;
	struct EventObserver* _Next = NULL;

	if(_Event->Type < 0 || _Event->Type >= EVENT_LAST)
		return;
	if((_Obs = RBSearch(g_EventHooks[_Event->Type], &_Event->Id)) != NULL) {
		do {
			_Next = _Obs->Next;
			_Obs->OnEvent(_Event, _Obs->Listener);
			_Obs = _Next;
		} while(_Obs != NULL);
	}
}

struct Event* HandleEvents() {
	struct Event* _Event = g_EventQueue.Top;

	if(g_EventQueue.Size <= 1) {
		g_EventQueue.Top = NULL;
		g_EventQueue.Bottom = NULL;
	} else
		g_EventQueue.Top = g_EventQueue.Top->Next;
	--g_EventQueue.Size;
	return _Event;
}

struct Event* CreateEventBirth(struct Person* _Mother, struct Person* _Child) {
	struct EventBirth* _Event = (struct EventBirth*) malloc(sizeof(struct EventBirth));

	EventCtor(struct EventBirth, _Event, _Mother->Id, (struct Location*)FamilyGetSettlement(_Mother->Family), EVENT_BIRTH);
	_Event->Mother = _Mother;
	_Event->Child = _Child;
	return (struct Event*)_Event;
}

struct Event* CreateEventDeath(struct Person* _Person) {
	struct EventDeath* _Event = (struct EventDeath*) malloc(sizeof(struct EventDeath));

	EventCtor(struct EventDeath, _Event, _Person->Id, (struct Location*)FamilyGetSettlement(_Person->Family), EVENT_AGE);
	_Event->Person = _Person;
	return (struct Event*)_Event;
}

struct Event* CreateEventTime(struct Person* _Person, DATE _Age) {
	struct EventAge* _Event = (struct EventAge*) malloc(sizeof(struct EventAge));
	struct Location* _Location = NULL;

	if(_Person != NULL)
		_Location = (struct Location*) _Person->Family->HomeLoc;
	EventCtor(struct EventAge, _Event, _Person->Id, _Location, EVENT_AGE);
	_Event->Person = _Person;
	_Event->Age = _Age;
	return (struct Event*)_Event;
}

struct Event* CreateEventFarming(int _Action, const struct Field* _Field) {
	struct EventFarming* _Event = (struct EventFarming*) malloc(sizeof(struct EventFarming));

	EventCtor(struct EventFarming, _Event, _Field->Id, (struct Location*)FamilyGetSettlement(_Field->Owner), EVENT_FARMING);
	_Event->Action = _Action;
	_Event->Field = _Field;

	return (struct Event*)_Event;
}

struct Event* CreateEventStarvingFamily(struct Family* _Family) {
	struct EventStarvingFamily* _Event = (struct EventStarvingFamily*) malloc(sizeof(struct EventStarvingFamily));

	EventCtor(struct EventStarvingFamily, _Event, _Family->Id, (struct Location*)FamilyGetSettlement(_Family), EVENT_STARVINGFAMILY);
	_Event->Family = _Family;
	return (struct Event*)_Event;
}

struct EventObserver* CreateEventObserver(int _EventType, int _ObjectId, void (*_OnEvent)(const void*, void*), const void* _Listener) {
	struct EventObserver* _Obs = (struct EventObserver*) malloc(sizeof(struct EventObserver));

	_Obs->EventType = _EventType;
	_Obs->ObjectId = _ObjectId;
	_Obs->OnEvent = _OnEvent;
	_Obs->Listener = _Listener;
	_Obs->Next = NULL;
	_Obs->Prev = NULL;
	return _Obs;
}
void DestroyEventObserver(struct EventObserver* _EventObs) {
	free(_EventObs);
}
