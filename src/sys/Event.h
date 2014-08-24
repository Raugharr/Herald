/*
 * Author: David Brotz
 * File: Event.h
 */

#ifndef __EVENT_H
#define __EVENT_H

#include "../World.h"

#define EVENT_BIRTHMSG "%s has given birth to %s.";
#define EVENT_DEATHMSG "%s has died.";

enum {
	EVENTBIRTH = 0,
	EVENTDEATH,
	EVENTAGE,
	EVENTLAST //Do not remove.
};

struct Event {
	int Type;
};

struct EventBirth {
	int Type;
	struct Person* Mother;
	struct Person* Child;
};

struct EventDeath {
	int Type;
	struct Person* Person;
};

struct EventAge {
	int Type;
	struct Person* Person;
	DATE Age;
};

void EventInit();
void EventQuit();
void EventPush(struct Event* _Event);
void EventHook(int _EventId, void (*_Callback)(struct Event*));
void EventRmHook(int _EventId, void(*_Callback)(struct Event*));

struct Event* CreateEventBirth(struct Person* _Mother, struct Person* _Child);
struct Event* CreateEventDeath(struct Person* _Person);
struct Event* CreateEventTime(struct Person* _Person, DATE _Age);

#endif

