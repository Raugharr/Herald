/*
 * Author: David Brotz
 * File: Event.c
 */

#include "Event.h"

#include "MemoryPool.h"
#include "LinkedList.h"
#include "Queue.h"
#include "RBTree.h"
#include "LuaCore.h"

#include "../video/Video.h"
#include "../video/GuiLua.h"

#include "../Person.h"
#include "../Family.h"
#include "../Crop.h"
#include "../BigGuy.h"
#include "../Battle.h"
#include "../Mission.h"

#include <lua/lauxlib.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define EVENTPOOL (1024)

static int g_EventTypes[EVENT_SIZE];
static struct KeyMouseState g_KeyMouseState = {0, 0, 0, 0, 0, 0, {0, 0}, 0};
static struct RBTree* g_EventHooks[EVENT_SIZE];
int g_EventId = 0;
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

/*int EventCmp(const void** _Vars, const struct EventObserver* _Event) {
	int _Result = _Vars[0] - _Event->One;

	if(_Result != 0)
		return _Result;
	if((_Result = _Vars[1] - _Event->Two) != 0)
		return _Result;
	return _Result;
}*/

int EventCmp(const void* _Owner, const struct EventObserver* _Event) {
	return _Owner - _Event->OwnerObj;
}

int EventInsert(const struct EventObserver* _One, const struct EventObserver* _Two) {
	int _Result = _One->EventType - _Two->EventType;
	
	if(_Result != 0)
		return _Result;
	if((_Result = _One->OwnerObj - _Two->OwnerObj) != 0)
		return _Result;
	if((_Result = _One->One - _Two->One) != 0)
		return _Result;
	if((_Result = _One->Two - _Two->Two) != 0)
		return _Result;
	return _Result;
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

void EventHook(int _EventType, EventCallback _Callback, void* _Owner, void* _Data1, void* _Data2) {
	struct EventObserver* _Obs = NULL;
	struct EventObserver* _New = NULL;
	struct RBNode* _Node = NULL;

	SDL_assert(_EventType >= 0 && _EventType < EVENT_SIZE);
	//SDL_assert(_EventType < g_EventTypes[0] || _EventType > g_EventTypes[EVENT_SIZE]);
	_New = CreateEventObserver(_EventType, _Callback, _Owner,  _Data1, _Data2); 
	if((_Node = RBSearchNode(g_EventHooks[_EventType], _Owner)) != NULL) {
		_New->Next = (struct EventObserver*) _Node->Data;
		_Node->Data = _New;
	} else {
		RBInsert(g_EventHooks[_EventType], _New);
	}
}

void EventHookRemove(int _EventType, void* _Owner, void* _Data1, void* _Data2) {
	struct EventObserver* _Obs = NULL;
	struct RBNode* _Node = NULL;

	SDL_assert(_EventType >= g_EventTypes[0] && _EventType <=  g_EventTypes[EVENT_SIZE - 1]);
	_EventType = _EventType -  g_EventTypes[0];
	if((_Node = RBSearchNode(g_EventHooks[_EventType], _Owner)) != NULL) {
		_Obs = _Node->Data;
		do {
			//One and Two should be equal by definition of being in the RB node.
			if(_Obs->One == _Data1 && _Obs->Two == _Data2) {
				ILL_DESTROY(_Node->Data, _Obs);
				DestroyEventObserver(_Obs);
				_Obs = _Node->Data;
				if(_Obs == NULL) {
					RBDeleteNode(g_EventHooks[_EventType], _Node);
					return;
				}
				continue;
			}
			_Obs = _Obs->Next;
		} while(_Obs != NULL);
	}
	assert(1);
}

void EventHookUpdate(const SDL_Event* _Event) {
	struct EventObserver* _Obs = NULL;
	void* _Vars[] = {_Event->user.data1, _Event->user.data2};

	SDL_assert(_Event->type  >= g_EventTypes[0] && _Event->type <=  g_EventTypes[EVENT_SIZE - 1]);
	_Obs = RBSearch(g_EventHooks[_Event->type - g_EventTypes[0]], _Event->user.data1/*_Vars*/);
	while(_Obs != NULL) {
		_Obs->OnEvent(_Event->type, _Obs->OwnerObj, _Obs->One, _Obs->Two);
		_Obs = _Obs->Next;
	}
}

void PushEvent(int _Type, void* _Data1, void* _Data2) {
	SDL_Event _Event;

	assert(_Type < EVENT_SIZE);
	_Event.type = g_EventTypes[_Type];
	_Event.user.data1 = _Data1;
	_Event.user.data2 = _Data2;
	SDL_PushEvent(&_Event);
}

struct EventObserver* CreateEventObserver(int _EventType, EventCallback _Callback, void* _Owner, void* _One, void* _Two) {
	struct EventObserver* _Obs = (struct EventObserver*) malloc(sizeof(struct EventObserver));

	_Obs->EventType = _EventType;
	_Obs->OnEvent = _Callback;
	_Obs->OwnerObj = _Owner;
	_Obs->One = _One;
	_Obs->Two = _Two;
	_Obs->Next = NULL;
	_Obs->Prev = NULL;
	return _Obs;
}
void DestroyEventObserver(struct EventObserver* _EventObs) {
	free(_EventObs);
}

void Events() {
	SDL_Event _Event;

	GUIMessageCheck(&g_GUIMessageList);
	KeyMouseStateClear(&g_KeyMouseState);
	SDL_GetMouseState(&g_KeyMouseState.MousePos.x, &g_KeyMouseState.MousePos.y);
	while(SDL_PollEvent(&_Event) != 0) {
		switch(_Event.type) {
		case SDL_KEYUP:
		case SDL_KEYDOWN:
			g_KeyMouseState.KeyboardState = _Event.key.state;
			g_KeyMouseState.KeyboardButton = _Event.key.keysym.sym;
			g_KeyMouseState.KeyboardMod = _Event.key.keysym.mod;
			break;
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
			g_KeyMouseState.MouseButton = _Event.button.button;
			g_KeyMouseState.MouseState = _Event.button.state;
			g_KeyMouseState.MouseClicks = _Event.button.clicks;
			g_KeyMouseState.MousePos.x = _Event.button.x;
			g_KeyMouseState.MousePos.y = _Event.button.y;
			break;
		case SDL_MOUSEMOTION:
			g_KeyMouseState.MouseMove = 1;
			g_KeyMouseState.MousePos.x = _Event.motion.x;
			g_KeyMouseState.MousePos.y = _Event.motion.y;
			break;
		}
		if(_Event.type >= g_EventTypes[0] && _Event.type <= g_EventTypes[EVENT_SIZE - 1]) {
			EventHookUpdate(&_Event);
			MissionOnEvent(&g_MissionEngine, _Event.type, _Event.user.data1);
		}
		if(_Event.type == g_EventTypes[EVENT_CRISIS]) {
			if(((struct BigGuy*)_Event.user.data2) == g_GameWorld.Player)
				MessageBox(g_LuaState, "A crisis has occured.");
		} else if(_Event.type == g_EventTypes[EVENT_FEUD]) {
			if(((struct BigGuy*)_Event.user.data2) == g_GameWorld.Player)
				MessageBox(g_LuaState, "A feud has occured.");
		} else if(_Event.type == g_EventTypes[EVENT_DEATH]) {
			if(_Event.user.data1 == g_GameWorld.Player->Person) {
				MessageBox(g_LuaState, "You have died.");
				
			}
		} else if(_Event.type == g_EventTypes[EVENT_BATTLE]) {
			struct Battle* _Battle = _Event.user.data1;
			char _Buffer[256];

			sprintf(_Buffer, "The attacker has lost %i men out of %i. The defender has lost %i men out of %i.",
					_Battle->Stats.AttkCas, _Battle->Stats.AttkBegin, _Battle->Stats.DefCas, _Battle->Stats.DefBegin);
			MessageBox(g_LuaState, _Buffer);
			DestroyBattle(_Battle);
		}
	}

	if(VideoEvents(&g_KeyMouseState) == 0 && g_GameWorld.IsPaused == 0)
		GameWorldEvents(&g_KeyMouseState, &g_GameWorld);
}

void GetMousePos(struct SDL_Point* _Point) {
	*_Point = g_KeyMouseState.MousePos;
}

int StringToEvent(const char* _Str) {
	for(int i = 0; g_EventNames[i] != NULL; ++i) {
		if(strcmp(_Str, g_EventNames[i]) == 0)
			return i;
	}
	return -1;
}

int EventUserOffset() {
	return g_EventTypes[0];
}
