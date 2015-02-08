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
	EVENT_BIRTH = 0,
	EVENT_DEATH,
	EVENT_AGE,
	EVENT_FARMING,
	EVENT_LAST //Do not remove.
};

struct Event {
	int Id;
	int Type;
	int X;
	int Y;
};

struct EventBirth {
	int Id;
	int Type;
	int X;
	int Y;
	struct Person* Mother;
	struct Person* Child;
};

struct EventDeath {
	int Id;
	int Type;
	int X;
	int Y;
	struct Person* Person;
};

struct EventAge {
	int Id;
	int Type;
	int X;
	int Y;
	struct Person* Person;
	DATE Age;
};

struct EventFarming {
	int Id;
	int Type;
	int X;
	int Y;
	int Action;
	const struct Field* Field;
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
struct Event* CreateEventFarming(int _X, int _Y, int _Action, const struct Field* _Field);

#endif

