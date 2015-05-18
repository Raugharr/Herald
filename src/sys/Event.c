/*
 * Author: David Brotz
 * File: Event.c
 */

#include "Event.h"

#include "MemoryPool.h"
#include "LinkedList.h"
#include "Queue.h"

#include "../Person.h"
#include "../Family.h"
#include "../Crop.h"

#include <stdlib.h>

#define EVENTPOOL (1024)

static struct EventQueue g_EventQueue = {0, NULL, NULL};
static struct LinkedList** g_EventHooks = NULL;
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
	return ((struct EventObserver*)((struct LinkedList*)_One)->Front->Data)->ObjectId - ((struct EventObserver*)((struct LinkedList*)_Two)->Front->Data)->ObjectId;
}

int ActorObserverS(const void* _One, const void* _Two) {
	return ((struct EventObserver*)((struct LinkedList*)_Two)->Front->Data)->ObjectId - *((int*)_Two);
}

struct RBTree g_ActorObservers = {NULL, 0, ActorObserverI, ActorObserverS};

#define EventCtor(_Struct, _Event, _Location, _Type)					\
	(_Event)->Id = g_EventId++;											\
	(_Event)->Location = (_Location);									\
	(_Event)->Type = (_Type);											\
	(_Event->Next) = NULL

void EventInit() {
	int i;
	g_EventHooks = (struct LinkedList**) malloc(sizeof(struct LinkedList**) * EVENT_LAST);
	for(i = 0; i < EVENT_LAST; ++i)
		g_EventHooks[i] = CreateLinkedList();
}

void EventQuit() {
	int i = 0;

	for(i = 0; i < EVENT_LAST; ++i)
		DestroyLinkedList(g_EventHooks[i]);
	free(g_EventHooks);
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

void EventHook(int _EventId, void (*_Callback)(struct Event*)) {
	if(_EventId < 0 && _EventId >= EVENT_LAST)
		return;
	LnkLst_PushBack(g_EventHooks[_EventId], &_Callback);
}

void EventRmHook(int _EventId, void(*_Callback)(struct Event*)) {
	struct LnkLst_Node* _Node = NULL;
	struct LinkedList* _List = NULL;

	if(_EventId < 0 && _EventId >= EVENT_LAST)
		return;
	_List = g_EventHooks[_EventId];
	_Node = _List->Front;
	if(*((void(*)(struct Event*))(_List->Front->Data)) == _Callback) {
		LnkLst_PopFront(_List);
		return;
	}
	_Node = _Node->Next;

	while(_Node != NULL) {
		if(*((void(*)(struct Event*))(_Node->Data)) == _Callback) {
			LnkLst_Remove(_List, _Node);
			return;
		}
		_Node = _Node->Next;
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

	EventCtor(struct EventBirth, _Event, (struct Location*)_Mother->Family->HomeLoc, EVENT_BIRTH);
	_Event->Mother = _Mother;
	_Event->Child = _Child;
	return (struct Event*)_Event;
}

struct Event* CreateEventDeath(struct Person* _Person) {
	struct EventDeath* _Event = (struct EventDeath*) malloc(sizeof(struct EventDeath));

	EventCtor(struct EventDeath, _Event, (struct Location*)_Person->Family->HomeLoc, EVENT_AGE);
	_Event->Person = _Person;
	return (struct Event*)_Event;
}

struct Event* CreateEventTime(struct Person* _Person, DATE _Age) {
	struct EventAge* _Event = (struct EventAge*) malloc(sizeof(struct EventAge));
	struct Location* _Location = NULL;

	if(_Person != NULL)
		_Location = (struct Location*) _Person->Family->HomeLoc;
	EventCtor(struct EventAge, _Event, _Location, EVENT_AGE);
	_Event->Person = _Person;
	_Event->Age = _Age;
	return (struct Event*)_Event;
}

struct Event* CreateEventFarming(int _Action, const struct Field* _Field) {
	struct EventFarming* _Event = (struct EventFarming*) malloc(sizeof(struct EventFarming));

	EventCtor(struct EventFarming, _Event, (struct Location*)_Field->Owner->HomeLoc, EVENT_FARMING);
	_Event->Action = _Action;
	_Event->Field = _Field;

	return (struct Event*)_Event;
}

struct Event* CreateEventStarvingFamily(struct Family* _Family) {
	struct EventStarvingFamily* _Event = (struct EventStarvingFamily*) malloc(sizeof(struct EventStarvingFamily));

	EventCtor(struct EventStarvingFamily, _Event, (struct Location*)_Family->HomeLoc, EVENT_STARVINGFAMILY);
	_Event->Family = _Family;
	return (struct Event*)_Event;
}

struct EventObserver* CreateEventObserver(int _EventType, int _ObjectId, void (*_OnEvent)(const void*, void*), const void* _Listener) {
	struct EventObserver* _Obs = (struct EventObserver*) malloc(sizeof(struct EventObserver));

	_Obs->EventType = _EventType;
	_Obs->ObjectId = _ObjectId;
	_Obs->OnEvent = _OnEvent;
	_Obs->Listener = _Listener;
	return _Obs;
}
void DestroyEventObserver(struct EventObserver* _EventObs) {
	free(_EventObs);
}

void ActorObserverInsert(struct EventObserver* _Obs) {
	struct LinkedList* _List = NULL;

	if((_List = RBSearch(&g_ActorObservers, &_Obs->EventType)) == NULL) {
		_List = CreateLinkedList();
		LnkLstPushBack(_List, _Obs);
		RBInsert(&g_ActorObservers, _List);
	} else
		LnkLstPushBack(_List, _Obs);
}

void ActorObserverRemove(struct EventObserver* _Obs) {
	struct LinkedList* _List = NULL;
	struct LnkLst_Node* _Itr = NULL;
	struct RBNode* _Node = RBSearchNode(&g_ActorObservers, _List);

	if(_Node == NULL)
		return;
	while(_Itr != NULL) {
		if(_Itr->Data == _Obs) {
			LnkLstRemove(_List, _Itr);
			break;
		}
		_Itr = _Itr->Next;
	}
	if(_List->Size == 0) {
		DestroyLinkedList(_List);
		RBDeleteNode(&g_ActorObservers, _Node);
	}
}
