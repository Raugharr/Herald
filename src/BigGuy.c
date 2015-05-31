/*
 * File: BigGuy.c
 * Author: David Brotz
 */

#include "BigGuy.h"

#include "Person.h"
#include "Family.h"
#include "World.h"
#include "Location.h"

#include "sys/RBTree.h"
#include "sys/Event.h"
#include "sys/LinkedList.h"

#include <stdlib.h>

const char* g_BGStateStr[BGSTATE_SIZE] = {
		"PassReform",
		"IsRuler"
};

void BGOnObserve(const struct EventDeath* _Event, struct BigGuy* _Guy) {
	_Guy->Person = NULL;
}

struct BigGuy* CreateBigGuy(struct Person* _Person) {
	struct BigGuy* _BigGuy = (struct BigGuy*) malloc(sizeof(struct BigGuy));

	_BigGuy->Person = _Person;
	_BigGuy->State = 0;
	_BigGuy->Authority = 0;
	_BigGuy->IsDirty = 1;
	RBInsert(&g_GameWorld.BigGuys, _BigGuy);
	RBInsert(&g_GameWorld.BigGuyStates, _BigGuy);
	ActorObserverInsert(CreateEventObserver(EVENT_DEATH, _Person->Id, ((void(*)(const void*, void*))BGOnObserve), _BigGuy));
	return _BigGuy;
}

void DestroyBigGuy(struct BigGuy* _BigGuy) {
	struct LinkedList* _List = &_BigGuy->Person->Family->HomeLoc->BigGuys;
	struct LnkLst_Node* _Itr = _List->Front;

	while(_Itr != NULL) {
		if(_Itr->Data == _BigGuy) {
			LnkLstRemove(_List, _Itr);
			break;
		}
		_Itr = _Itr->Next;
	}
	RBDelete(&g_GameWorld.BigGuys, _BigGuy);
	RBDelete(&g_GameWorld.BigGuyStates, _BigGuy);
	free(_BigGuy);
}

int BigGuyIdInsert(const struct BigGuy* _One, const struct BigGuy* _Two) {
	return _One->Person->Id - _Two->Person->Id;
}

int BigGuyIdCmp(const struct BigGuy* _BigGuy, const int* _Two) {
	return _BigGuy->Person->Id - (*_Two);
}

int BigGuyStateInsert(const struct BigGuy* _One, const struct BigGuy* _Two) {
	return _One->State - _Two->State;
}

int BigGuyMissionCmp(const struct BigGuy* _BigGuy, const struct Mission* _Mission) {
	return 0;
}

struct BigGuy* BigGuyLeaderType(struct Person* _Person) {
	while(_Person != NULL) {
		if(_Person->Gender == EMALE && DateToDays(_Person->Age) > ADULT_AGE) {
			return CreateBigGuy(_Person); //NOTE: Make sure we aren't making a big guy when the person is already a big guy.
		}
		_Person = _Person->Next;
	}
	return NULL;
}
