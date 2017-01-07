/*
 * Author: David Brotz
 * File: Event.h
 */

#ifndef __EVENT_H
#define __EVENT_H

#include "../World.h"

#include "RBTree.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

extern const char* g_EventNames[];
extern struct RBTree g_ActorObservers;

struct Location;
struct MissionEngine;
struct EventData;
struct Faction;

/**
 * Function prototype for all functions that are used in event callbacks.
 * The first argument contains the type of event, the owner of the event,
 * pointers who's value is set when the event was hooked. The second argument
 * contains a pointer to data pushed in EventPush. */
typedef void (*EventCallback)(const struct EventData*, void*, void*);

enum {
	EVENT_CRISIS = 0,
	EVENT_BIRTH,
	EVENT_BATTLE,
	EVENT_DEATH,
	EVENT_AGE,
	EVENT_FARMING,
	EVENT_STARVINGFAMILY,
	EVENT_SLANDER,
	EVENT_NEWLEADER,
	EVENT_NEWPLOT,
	EVENT_ENDPLOT,
	EVENT_NEWPOLICY,
	EVENT_CHANGEPOLICY,
	EVENT_JOINRETINUE,
	EVENT_QUITRETINUE,
	EVENT_BATTLESTART,
	EVENT_BATTLEEND,
	EVENT_MURDERED,
	EVENT_TAKERET,
	EVENT_WARBNDHOME,
	EVENT_SIZE
};

#define KeyMouseStateClear(State)	\
	(State)->MouseButton = -1;		\
	(State)->MouseState = -1;		\
	(State)->MouseClicks = 0;		\
	(State)->KeyboardButton = 0;	\
	(State)->KeyboardMod = 0;		\
	(State)->MouseMove = 0;		\
	(State)->MousePos.x = -1;		\
	(State)->MousePos.y = -1;		\
	(State)->KeyboardState = 0;

struct KeyMouseState {
	uint16_t MouseButton; /* Which button is pressed. */
	uint16_t MouseState; /* Pressed or released. */
	uint16_t MouseClicks;
	uint16_t KeyboardButton; /* Which key is pressed. */
	uint16_t KeyboardMod;
	uint16_t MouseMove;
	int16_t KeyboardState; /* SDL_PRESSED or SDL_RELEASED */
	SDL_Point MousePos;
};

struct WEvent {
	struct KeyMouseState Event; //TODO: All the data is checked for an event not just relevant data. This means if we want to click a widget it will fail if we are clicking and pressing a button etc.
	int WidgetId;
	int RefId;
};

struct EventData {
	uint32_t EventType;
	void* OwnerObj;
	void* One;
	void* Two;
};

struct EventObserver {
	uint32_t EventType;
	void* OwnerObj;
	void* One;
	void* Two;
	EventCallback OnEvent; 
	struct EventObserver* Next;
	struct EventObserver* Prev;
};

/*
 * TODO: Merge EventHook and ActorObserver.
 * Have the EventHook's Callback also have a parameter for a listener.
 */
void EventInit();
void EventQuit();
void EventHook(int EventType, EventCallback Callback, void* Owner, void* Data1, void* Data2);
void EventHookRemove(int EventType, void* Owner, void* Data1, void* Data);
void EventHookUpdate(const SDL_Event* Event);

void PushEvent(uint32_t Type, void* Data1, void* Data2);
struct EventObserver* CreateEventObserver(int EventType, EventCallback Callback, void* Owner, void* One, void* Two);
void DestroyEventObserver(struct EventObserver* EventObs);
void Events();
void GetMousePos(struct SDL_Point* Point);
int StringToEvent(const char* Str);
int EventUserOffset();

void EventFactionGoalEnd(struct Faction* Faction, uint32_t Goal, lua_State* State);

#endif

