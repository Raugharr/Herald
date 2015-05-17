/*
 * Author: David Brotz
 * File: Event.h
 */

#ifndef __EVENT_H
#define __EVENT_H

#include "../World.h"
#include "RBTree.h"

#define EVENT_BIRTHMSG "%s has given birth to %s.";
#define EVENT_DEATHMSG "%s has died.";

#ifndef NULL
#define NULL ((void*)0)
#endif

extern const char* g_EventNames[];
extern struct RBTree g_ActorObservers;

struct Location;

enum {
	EVENT_BIRTH = 0,
	EVENT_DEATH,
	EVENT_AGE,
	EVENT_FARMING,
	EVENT_STARVINGFAMILY,
	EVENT_DATE,
	EVENT_LAST //Do not remove.
};

struct EventObserver {
	int EventType;
	int ObjectId;
	void (*OnEvent)(const void*, void*); //First const void* is the event, second const void* is the listener.
	const void* Listener;
};

struct EventQueue {
	int Size;
	struct Event* Top;
	struct Event* Bottom;
};

struct Event {
	int Id;
	int Type;
	struct Event* Next;
	struct Location* Location;
};

struct EventBirth {
	int Id;
	int Type;
	struct Event* Next;
	struct Location* Location;
	struct Person* Mother;
	struct Person* Child;
};

struct EventDeath {
	int Id;
	int Type;
	struct Event* Next;
	struct Location* Location;
	struct Person* Person;
};

struct EventAge {
	int Id;
	int Type;
	struct Event* Next;
	struct Location* Location;
	struct Person* Person;
	DATE Age;
};

struct EventFarming {
	int Id;
	int Type;
	struct Event* Next;
	struct Location* Location;
	int Action;
	const struct Field* Field;
};

struct EventStarvingFamily {
	int Id;
	int Type;
	struct Event* Next;
	struct Location* Location;
	struct Family* Family;
};


void EventInit();
void EventQuit();
void EventPush(struct Event* _Event);
void EventHook(int _EventId, void (*_Callback)(struct Event*));
void EventRmHook(int _EventId, void(*_Callback)(struct Event*));
struct Event* HandleEvents();

struct Event* CreateEventBirth(struct Person* _Mother, struct Person* _Child);
struct Event* CreateEventDeath(struct Person* _Person);
struct Event* CreateEventTime(struct Person* _Person, DATE _Age);
struct Event* CreateEventFarming(int _Action, const struct Field* _Field);
struct Event* CreateEventStarvingFamily(struct Family* _Family);

struct EventObserver* CreateEventObserver(int _EventType, int _ObjectId, void (*_OnEvent)(const void*, void*), const void* _Listener);
void DestroyEventObserver(struct EventObserver* _EventObs);

void ActorObserverInsert(struct EventObserver* _Obs);
void ActorObserverRemove(struct EventObserver* _Obs);

#endif

