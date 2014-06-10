/*
 * Author: David Brotz
 * File: EventPerson.h
 */

 #ifndef __EVENTPERSON_H
 #define __EVENTPERSON_H

extern char g_EventBirthMsg[];
extern char g_EventDeathMsg[];

struct Event;

struct EventBirth {
	int Id;
	struct Person* Mother;
	struct Person* Child;
};

struct EventDeath {
	int Id;
	struct Person* Person;
};

//Person decides to leave heir current settlement.
struct EventLeave {
	int Id;
	struct Person* Person;
	struct Manor* Leave;
};

//Person deciddes to join another settlement.
struct EventEnter {
	int Id;
	struct Person* Person;
	struct Manor* Enter;
};

struct Event* CreateEventBirth(struct Person* _Mother, struct Person* _Child);
struct Event* CreateEventDeath(struct Person* _Person);
#endif

