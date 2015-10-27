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
	EVENT_DEATH = 0,
	EVENT_AGE,
	EVENT_FARMING,
	EVENT_STARVINGFAMILY,
	EVENT_DATE,
	EVENT_LAST //Do not remove.
};

enum {
	EVENT_CRISIS = 0,
	EVENT_FEUD,
	EVENT_BIRTH,
	EVENT_BATTLE,
	EVENT_SIZE
};

#define KeyMouseStateClear(_State)	\
	(_State)->MouseButton = -1;		\
	(_State)->MouseState = -1;		\
	(_State)->MouseClicks = 0;		\
	(_State)->KeyboardButton = 0;	\
	(_State)->KeyboardMod = 0;		\
	(_State)->MouseMove = 0;		\
	(_State)->MousePos.x = -1;		\
	(_State)->MousePos.y = -1;		\
	(_State)->KeyboardState = 0;

struct KeyMouseState {
	unsigned int MouseButton; /* Which button is pressed. */
	unsigned int MouseState; /* Pressed or released. */
	unsigned int MouseClicks;
	unsigned int KeyboardButton; /* Which key is pressed. */
	unsigned int KeyboardMod;
	unsigned int MouseMove;
	SDL_Point MousePos;
	int KeyboardState; /* Pressed or released. */
};

struct WEvent {
	struct KeyMouseState Event; //TODO: All the data is checked for an event not just relevant data. This means if we want to click a widget it will fail if we are clicking and pressing a button etc.
	int WidgetId;
	int RefId;
};

struct EventObserver {
	int EventType;
	int ObjectId;
	void (*OnEvent)(const void*, void*); //First const void* is the event, second const void* is the listener.
	void* Listener;
	struct EventObserver* Next;
	struct EventObserver* Prev;
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
	int ObjId;
};

struct EventDeath {
	int Id;
	int Type;
	struct Event* Next;
	int ObjId;
	struct Location* Location;
	struct Person* Person;
};

struct EventAge {
	int Id;
	int Type;
	struct Event* Next;
	int ObjId;
	struct Location* Location;
	struct Person* Person;
	DATE Age;
};

struct EventFarming {
	int Id;
	int Type;
	struct Event* Next;
	int ObjId;
	struct Location* Location;
	int Action;
	const struct Field* Field;
};

struct EventStarvingFamily {
	int Id;
	int Type;
	struct Event* Next;
	int ObjId;
	struct Location* Location;
	struct Family* Family;
};

/*
 * TODO: Merge EventHook and ActorObserver.
 * Have the EventHook's Callback also have a parameter for a listener.
 */
void EventInit();
void EventQuit();
void EventPush(struct Event* _Event);
void EventHook(int _EventType, int _ObjId, void (*_OnEvent)(const void*, void*), void* _Listener);
void EventHookRemove(int _EventType, int _ObjId);
void EventHookUpdate(const struct Event* _Event);
struct Event* HandleEvents();

struct Event* CreateEventTime(struct Person* _Person, DATE _Age);
struct Event* CreateEventFarming(int _Action, const struct Field* _Field);
struct Event* CreateEventStarvingFamily(struct Family* _Family);

void PushEvent(int _Type, void* _Data1, void* _Data2);
struct EventObserver* CreateEventObserver(int _EventType, int _ObjectId, void (*_OnEvent)(const void*, void*), const void* _Listener);
void DestroyEventObserver(struct EventObserver* _EventObs);
void Events(void);
void GetMousePos(struct SDL_Point* _Point);

#endif

