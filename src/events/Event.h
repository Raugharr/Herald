/*
 * Author: David Brotz
 * File: Event.h
 */

#ifndef __EVENT_H
#define __EVENT_H

#include "EventPerson.h"

enum {
	EVENTBIRTH = 0,
	EVENTDEATH,
	EVENTLEAVE,
	EVENTENTER,
	EVENTHARVEST,
	EVENTPLANT,
	EVENTLAST //Do not remove.
};

struct Event {
	int Id;
};

void Event_Init();
void Event_Quit();
void Event_Push(struct Event* _Event);
void Event_Hook(int _EventId, void (*_Callback)(struct Event*));
void Event_RemoveHook(int _EventId, void(*_Callback)(struct Event*));
#endif

