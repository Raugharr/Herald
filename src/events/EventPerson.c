/*
 * Author: David Brotz
 * File: EventPerson.c
 */

#include "EventPerson.h"

#include "Event.h"

#include <stdlib.h>

char g_EventBirthMsg[] = "%s has given birth to %s.";
char g_EventDeathMsg[] = "%s has died.";

struct Event* CreateEventBirth(struct Person* _Mother, struct Person* _Child) {
	struct EventBirth* _EventBirth = (struct EventBirth*) malloc(sizeof(struct EventBirth));

	_EventBirth->Id = EVENTBIRTH;
	_EventBirth->Mother = _Mother;
	_EventBirth->Child = _Child;
	return ((struct Event*)_EventBirth);
}

struct Event* CreateEventDeath(struct Person* _Person) {
	struct EventDeath* _EventDeath = (struct EventDeath*) malloc(sizeof(struct EventDeath));

	_EventDeath->Id = EVENTDEATH;
	_EventDeath->Person = _Person;
	return ((struct Event*)_EventDeath);
}
