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
	EVENTHARVEST
};

struct Event {
	int Id;
};

void Event_Init();
void Event_Quit();
void Event_Push(struct Event* _Event);
#endif

