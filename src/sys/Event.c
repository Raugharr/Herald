/*
 * Author: David Brotz
 * File: Event.c
 */

#include "Event.h"

#include "MemoryPool.h"
#include "StackAllocator.h"
#include "LinkedList.h"
#include "Queue.h"
#include "RBTree.h"
#include "LuaCore.h"
#include "Log.h"

#include "../World.h"
#include "../Mission.h"

#include "../video/Gui.h"
#include "../video/Video.h"
#include "../video/GuiLua.h"

#include <lua/lauxlib.h>
#include <stdlib.h>
#include <string.h>

#define EVENTPOOL (1024)

static int g_EventTypes[EVENT_SIZE];
static struct KeyMouseState g_KeyMouseState = {0};
static struct RBTree* g_EventHooks[EVENT_SIZE];
unsigned int g_EventId = 0;
const char* g_EventNames[] = {
		"Crisis",
		"Feud",
		"Birth",
		"Battle",
		"Death",
		"Age",
		"Farming",
		"StarvingFamily",
		"SabatogeRelations",
		NULL
};

static EventGeneral g_EventCallbacks [EVENT_SIZE] = {NULL};

/*int EventCmp(const void** Vars, const struct EventObserver* Event) {
	int Result = Vars[0] - Event->One;

	if(Result != 0)
		return Result;
	if((Result = Vars[1] - Event->Two) != 0)
		return Result;
	return Result;
}*/

int EventCmp(const void* Owner, const struct EventObserver* Event) {
	return Owner - Event->OwnerObj;
}

int EventInsert(const struct EventObserver* One, const struct EventObserver* Two) {
	int Result = One->EventType - Two->EventType;
	
	if(Result != 0)
		return Result;
	if((Result = One->OwnerObj - Two->OwnerObj) != 0)
		return Result;
	if((Result = One->One - Two->One) != 0)
		return Result;
	if((Result = One->Two - Two->Two) != 0)
		return Result;
	return Result;
}

void EventInit() {
	for(int i = 0; i < EVENT_SIZE; ++i)
		g_EventHooks[i] = CreateRBTree((int(*)(const void*, const void*)) EventInsert, (int(*)(const void*, const void*)) EventCmp);
	g_EventTypes[0] = SDL_RegisterEvents(EVENT_SIZE);
	for(int i = 1; i < EVENT_SIZE; ++i)
		g_EventTypes[i] = g_EventTypes[0] + i;
}

void EventQuit() {
	for(int i = 0; i < EVENT_SIZE; ++i)
		DestroyRBTree(g_EventHooks[i]);
}

void EventClear() {
	for(int i = 0; i < EVENT_SIZE; ++i)
		RBRemoveAll(g_EventHooks[i], (void(*)(void*))DestroyEventObserver);
}

void EventHook(int EventType, EventCallback Callback, void* Owner, void* Data1, void* Data2) {
	struct EventObserver* New = NULL;
	struct RBNode* Node = NULL;

	SDL_assert(EventType >= 0 && EventType < EVENT_SIZE);
	New = CreateEventObserver(EventType, Callback, Owner,  Data1, Data2); 
	if((Node = RBSearchNode(g_EventHooks[EventType], Owner)) != NULL) {
		New->Next = (struct EventObserver*) Node->Data;
		New->Next->Prev = New;
		Node->Data = New;
	} else {
		RBInsert(g_EventHooks[EventType], New);
	}
}

void EventHookRemove(int EventType, void* Owner, void* Data1, void* Data2) {
	struct EventObserver* Obs = NULL;
	struct RBNode* Node = NULL;

	Assert(EventType >= 0 && EventType <=  EVENT_SIZE);
	if((Node = RBSearchNode(g_EventHooks[EventType], Owner)) != NULL) {
		Obs = Node->Data;
		do {
			//One and Two should be equal by definition of being in the RB node.
			if(Obs->One == Data1 && Obs->Two == Data2) {
				struct EventObserver* Next = Obs->Next;

				ILL_DESTROY(Node->Data, Obs);
				if(Node->Data == NULL) {
					RBDeleteNode(g_EventHooks[EventType], Node);
					DestroyEventObserver(Obs);
					return;
				}
				DestroyEventObserver(Obs);
				Obs = Next;
				continue;
			}
			Obs = Obs->Next;
		} while(Obs != NULL);
	}
}

void EventHookUpdate(const SDL_Event* Event) {
	struct EventObserver* Obs = NULL;

	SDL_assert(Event->type  >= g_EventTypes[0] && Event->type <=  g_EventTypes[EVENT_SIZE - 1]);
	Obs = RBSearch(g_EventHooks[Event->type - g_EventTypes[0]], Event->user.data1);
	while(Obs != NULL) {
		Obs->OnEvent((struct EventData*)Obs, Event->user.data1, Event->user.data2);
		Obs = Obs->Next;
	}
}

void PushEvent(uint32_t Type, void* Data1, void* Data2) {
	SDL_Event Event;

	Assert(Type < EVENT_SIZE);
	Assert(Type >= 0);
	Event.type = g_EventTypes[Type];
	Event.user.data1 = Data1;
	Event.user.data2 = Data2;
	SDL_PushEvent(&Event);
}

struct EventObserver* CreateEventObserver(int EventType, EventCallback Callback, void* Owner, void* One, void* Two) {
	struct EventObserver* Obs = (struct EventObserver*) malloc(sizeof(struct EventObserver));

	Obs->EventType = EventType;
	Obs->OnEvent = Callback;
	Obs->OwnerObj = Owner;
	Obs->One = One;
	Obs->Two = Two;
	Obs->Next = NULL;
	Obs->Prev = NULL;
	return Obs;
}
void DestroyEventObserver(struct EventObserver* EventObs) {
	free(EventObs);
}

void Events() {
	SDL_Event Event;

	KeyMouseStateClear(&g_KeyMouseState);
	SDL_GetMouseState(&g_KeyMouseState.MousePos.x, &g_KeyMouseState.MousePos.y);
	while(SDL_PollEvent(&Event) != 0) {
		switch(Event.type) {
		case SDL_KEYUP:
		case SDL_KEYDOWN:
			g_KeyMouseState.KeyboardState = Event.key.state;
			g_KeyMouseState.KeyboardButton = Event.key.keysym.sym;
			g_KeyMouseState.KeyboardMod = Event.key.keysym.mod;
			break;
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
			g_KeyMouseState.MouseButton = Event.button.button;
			g_KeyMouseState.MouseState = Event.button.state;
			g_KeyMouseState.MouseClicks = Event.button.clicks;
			g_KeyMouseState.MousePos.x = Event.button.x;
			g_KeyMouseState.MousePos.y = Event.button.y;
			break;
		case SDL_MOUSEMOTION:
			g_KeyMouseState.MouseMove = 1;
			g_KeyMouseState.MousePos.x = Event.motion.x;
			g_KeyMouseState.MousePos.y = Event.motion.y;
			break;
		case SDL_TEXTINPUT:
			strcpy(g_KeyMouseState.TextBuff, Event.text.text);
			Assert(g_KeyMouseState.TextBuff[0] != '\0');
			//VideoTextInput(g_LuaState, Event.text.text, g_KeyMouseState.KeyboardMod);
			break;
		}
		if(Event.type >= g_EventTypes[0] && Event.type <= g_EventTypes[EVENT_SIZE - 1]) {
			struct Person* Person = Event.user.data1;
			struct BigGuy* Guy = RBSearch(&g_GameWorld.BigGuys, Person);
			int EventId = Event.type - EventUserOffset();

			EventHookUpdate(&Event);
			if(Guy != NULL)
				MissionOnEvent(&g_MissionEngine, Event.type - EventUserOffset(), Guy, Event.user.data2);
			if(g_EventCallbacks[EventId] != NULL) g_EventCallbacks[EventId](Event.user.data1, Event.user.data2);
		}
	}
	//Code to relead a menu.
	/*
	if(g_KeyMouseState.KeyboardButton == SDLK_r && g_KeyMouseState.KeyboardState == SDL_RELEASED) {
		const char* GuiMenu = StackTop(&g_GUIStack);
		char* Menu = alloca(sizeof(char) * strlen(GuiMenu));
		char* File = alloca(sizeof(char) * strlen(GuiMenu) + 5);//+ 1 for \0 and 4 for .lua

		chdir("data/gui");
		strcpy(Menu, GuiMenu);
		strcpy(File, Menu);
		strcat(File, ".lua");
		if(GuiLoadMenu(g_LuaState, File) == true) {
			lua_settop(g_LuaState, 0);
			LuaCloseMenu(g_LuaState);
			lua_settop(g_LuaState, 0);
			lua_pushstring(g_LuaState, Menu);
			lua_getglobal(g_LuaState, Menu);
			lua_pushstring(g_LuaState, "__input");
			lua_rawget(g_LuaState, -2);
			lua_remove(g_LuaState, -2);
			LuaSetMenu(g_LuaState);
		}
		chdir("../..");
	}*/
	if(VideoEvents(&g_KeyMouseState) == 0)
		GameWorldEvents(&g_KeyMouseState, &g_GameWorld);
}

void GetMousePos(struct SDL_Point* Point) {
	*Point = g_KeyMouseState.MousePos;
}

int StringToEvent(const char* Str) {
	for(int i = 0; g_EventNames[i] != NULL; ++i) {
		if(strcmp(Str, g_EventNames[i]) == 0)
			return i;
	}
	return -1;
}

int EventUserOffset() {
	return g_EventTypes[0];
}

void EventFactionGoalEnd(struct Faction* Faction, uint32_t Goal, lua_State* State) {
	lua_settop(State, 0);
	lua_pushstring(State, "FactionBetEnd");
	lua_createtable(State, 0, 1);
	lua_pushstring(State, "Faction");
	LuaCtor(State, Faction, LOBJ_FACTION);
	lua_rawset(State, -3);
	lua_pushstring(State, "Goal");
	lua_pushinteger(State, Goal);
	lua_rawset(State, -3);
	LuaCreateWindow(State);	
}

void EventSetCallback(unsigned int EventId, EventGeneral Callback) {
	if(EventId >= EVENT_SIZE) return;
	g_EventCallbacks[EventId] = Callback;
}
