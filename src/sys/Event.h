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

typedef void (*EventCallback)(int, void*, void*, void*); 

enum {
	EVENT_CRISIS = 0,
	EVENT_FEUD,
	EVENT_BIRTH,
	EVENT_BATTLE,
	EVENT_DEATH,
	EVENT_AGE,
	EVENT_FARMING,
	EVENT_STARVINGFAMILY,
	EVENT_SABRELATION,
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
	EventCallback OnEvent; //First const void* is the event, second const void* is the listener.
	void* OwnerObj;
	void* One;
	void* Two;
	struct EventObserver* Next;
	struct EventObserver* Prev;
};

/*
 * TODO: Merge EventHook and ActorObserver.
 * Have the EventHook's Callback also have a parameter for a listener.
 */
void EventInit();
void EventQuit();
void EventHook(int _EventType, EventCallback _Callback, void* _Owner, void* _Data1, void* _Data2);
void EventHookRemove(int _EventType, void* _Owner, void* _Data1, void* _Data);
void EventHookUpdate(const SDL_Event* _Event);

void PushEvent(int _Type, void* _Data1, void* _Data2);
struct EventObserver* CreateEventObserver(int _EventType, EventCallback _Callback, void* _Owner, void* _One, void* _Two);
void DestroyEventObserver(struct EventObserver* _EventObs);
void Events();
void GetMousePos(struct SDL_Point* _Point);
int StringToEvent(const char* _Str);
int EventUserOffset();

#endif

