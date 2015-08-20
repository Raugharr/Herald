/*
 * File: BigGuy.c
 * Author: David Brotz
 */

#include "BigGuy.h"

#include "Person.h"
#include "Family.h"
#include "World.h"
#include "Location.h"

#include "ai/Utility.h"
#include "ai/goap.h"
#include "ai/Setup.h"
#include "ai/Agent.h"

#include "sys/Math.h"
#include "sys/Constraint.h"

#include "sys/RBTree.h"
#include "sys/Event.h"
#include "sys/LinkedList.h"

#include <stdlib.h>

#define RELATIONS_PER_TICK (2)

const char* g_BGStateStr[BGBYTE_SIZE] = {
		"PassReform",
		"IsLeader",
		"ImproveRelations",
		"Authority",
		"Prestige",
		"FyrdRaised"
};

const char* g_BGMission[BGACT_SIZE] = {
		"Improve Relations"
};

void BigGuyActionImproveRel(struct BigGuy* _Guy, struct BigGuy* _Target) {
	struct BigGuyRelation* _Relation = NULL;

	if((_Relation = BigGuyGetRelation(_Target, _Guy)) == NULL)
		_Relation = CreateBigGuyRelation(_Target, _Guy, BGOPIN_IMPROVREL, RELATIONS_PER_TICK);
	else if(_Relation->Relation < BGREL_LIKE)
		BigGuyAddRelation(_Target, _Relation, BGOPIN_IMPROVREL, RELATIONS_PER_TICK);
}

void BGOnObserve(const struct EventDeath* _Event, struct BigGuy* _Guy) {
	_Guy->Person = NULL;
	EventHookRemove(_Event->Id, _Guy->Person->Id);
}

struct BigGuyOpinion* CreateBigGuyOpinion(struct BigGuyRelation* _Relation, int _Action, int _Modifier) {
	struct BigGuyOpinion* _Opinion = (struct BigGuyOpinion*) malloc(sizeof(struct BigGuyOpinion));

	_Opinion->Action = _Action;
	_Opinion->RelMod = _Modifier;
	_Opinion->Next = _Relation->Opinions;
	_Relation->Opinions = _Opinion;
	return _Opinion;
}

struct BigGuyRelation* CreateBigGuyRelation(struct BigGuy* _Guy, const struct BigGuy* _Actor, int _Action, int _Modifier) {
	struct BigGuyRelation* _Relation = (struct BigGuyRelation*) malloc(sizeof(struct BigGuyRelation));

	_Relation->Next = _Guy->Relations;
	_Guy->Relations = _Relation;
	_Relation->Opinions = NULL;
	CreateBigGuyOpinion(_Relation, _Action, _Modifier);
	_Relation->Person = _Actor;
	BigGuyRelationUpdate(_Relation);
	return _Relation;
}

struct BigGuy* CreateBigGuy(struct Person* _Person, struct BigGuyStats* _Stats) {
	struct BigGuy* _BigGuy = (struct BigGuy*) malloc(sizeof(struct BigGuy));

	_BigGuy->Person = _Person;
	WorldStateClear(&_BigGuy->State);
	_BigGuy->Authority = 0;
	_BigGuy->Prestige = 0;
	_BigGuy->IsDirty = 1;
	_BigGuy->Relations = NULL;
	_BigGuy->Stats = *_Stats;
	_BigGuy->Action.ActionFunc = NULL;
	_BigGuy->Action.Data = NULL;
	RBInsert(&g_GameWorld.BigGuys, _BigGuy);
	RBInsert(&g_GameWorld.BigGuyStates, _BigGuy);
	RBInsert(&g_GameWorld.Agents, CreateAgent(_BigGuy));
	LnkLstPushBack(&FamilyGetSettlement(_Person->Family)->BigGuys, _BigGuy);
	EventHook(EVENT_DEATH, _Person->Id, ((void(*)(const void*, void*))BGOnObserve), _BigGuy);
	CreateObject((struct Object*)_BigGuy, OBJECT_BIGGUY, (void(*)(struct Object*))BigGuyThink);
	return _BigGuy;
}

void DestroyBigGuy(struct BigGuy* _BigGuy) {
	struct LinkedList* _List = &FamilyGetSettlement(_BigGuy->Person->Family)->BigGuys;
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
	DestroyObject((struct Object*)_BigGuy);
	free(_BigGuy);
}

void BigGuyThink(struct BigGuy* _Guy) {
	struct BigGuyRelation* _Relation = _Guy->Relations;
	struct BigGuyOpinion* _Opinion = NULL;

	if(_Guy->Action.ActionFunc != NULL)
		_Guy->Action.ActionFunc(_Guy, _Guy->Action.Data);
	while(_Relation != NULL) {
		_Opinion = _Relation->Opinions;
		while(_Opinion != NULL) {
			_Opinion->RelMod = _Opinion->RelMod - AbsAdd(_Opinion->RelMod, 1);
			_Opinion = _Opinion->Next;
		}
		BigGuyRelationUpdate(_Relation);
		_Relation = _Relation->Next;
	}
}

int BigGuyIdInsert(const struct BigGuy* _One, const struct BigGuy* _Two) {
	return _One->Person->Id - _Two->Person->Id;
}

int BigGuyIdCmp(const int* _Two, const struct BigGuy* _BigGuy) {
	return (*_Two) - _BigGuy->Person->Id;
}

int BigGuyStateInsert(const struct BigGuy* _One, const struct BigGuy* _Two) {
	return WorldStateCmp(&_One->State, &_Two->State);
}

int BigGuyMissionCmp(const struct BigGuy* _BigGuy, const struct Mission* _Mission) {
	return 0;
}

void BigGuySetState(struct BigGuy* _Guy, int _State, int _Value) {
	WorldStateSetAtom(&_Guy->State, _State, _Value);
	_Guy->IsDirty = 1;
}

struct BigGuy* BigGuyLeaderType(struct Person* _Person) {
	while(_Person != NULL) {
		if(_Person->Gender == EMALE && DateToDays(_Person->Age) > ADULT_AGE) {
			struct BigGuyStats _Stats;

			BGStatsWarlord(&_Stats, 50);
			return CreateBigGuy(_Person, &_Stats); //NOTE: Make sure we aren't making a big guy when the person is already a big guy.
		}
		_Person = _Person->Next;
	}
	return NULL;
}

void BigGuyAddRelation(struct BigGuy* _Guy, struct BigGuyRelation* _Relation, int _Action, int _Modifier) {
	struct BigGuyOpinion* _Opinion = _Relation->Opinions;

	while(_Opinion != NULL) {
		if(_Opinion->Action == _Action) {
			_Opinion->RelMod += _Modifier;
			goto add_mods;
		}
		_Opinion = _Opinion->Next;
	}
	CreateBigGuyOpinion(_Relation, _Action, _Modifier);
	add_mods:
	_Relation->Modifier = _Relation->Modifier + _Modifier;
	_Relation->Relation = Fuzify(g_OpinionMods, _Relation->Modifier);
}

void BigGuyRelationUpdate(struct BigGuyRelation* _Relation) {
	struct BigGuyOpinion* _Opinion = _Relation->Opinions;
	int _Modifier = 0;

	while(_Opinion != NULL) {
		_Modifier += _Opinion->RelMod;
		_Opinion = _Opinion->Next;
	}
	_Relation->Relation = Fuzify(g_OpinionMods, _Modifier);
	_Relation->Modifier = _Modifier;
}

struct BigGuyRelation* BigGuyGetRelation(const struct BigGuy* _Guy, const struct BigGuy* _Target) {
	struct BigGuyRelation* _Relation = _Guy->Relations;

	while(_Relation != NULL) {
		if(_Relation->Person == _Target)
			return _Relation;
		_Relation = _Relation->Next;
	}
	return NULL;
}

void BGStatsRandom(int _Points, int _StatCt, ...) {
	va_list _Valist;
	uint8_t* _Stats[_StatCt];
	uint8_t* _Temp = NULL;
	int _Rand = 0;
	int _Result = 0;
	int _PointsLeft = _Points;
	double _StatDist[_StatCt];

	va_start(_Valist, _StatCt);
	for(int i = 0; i < _StatCt; ++i)
		_Stats[i] = va_arg(_Valist, uint8_t*);
	for(int i = 0; i < _StatCt; ++i)
		_StatDist[i] = va_arg(_Valist, double);
	for(int i = 0; i < _StatCt; ++i) {
		_Temp = _Stats[i];
		_Rand = Random(0, _StatCt - 1);
		_Stats[i] = _Stats[_Rand];
		_Stats[_Rand] = _Temp;
	}
	for(int i = 0; i < _StatCt; ++i) {
		_Result = ceil(_Points * _StatDist[i]);
		if(_Result > BIGGUYSTAT_MAX)
			_Result = BIGGUYSTAT_MAX;
		if(_PointsLeft < _Result)
			_Result = _PointsLeft;
		*_Stats[i] = _Result;
		_PointsLeft -= _Result;
	}
	va_end(_Valist);
}

void BGStatsWarlord(struct BigGuyStats* _Stats, int _Points) {
	int _WarPoints = _Points / 2;
	int _RemainPoints = _Points - _WarPoints;

	/*
	 * TODO: The percentages given to each stat should be randomized slightly.
	 */
	BGStatsRandom(_WarPoints, 3, &_Stats->Strategy, &_Stats->Tactics, &_Stats->Warfare, 0.5, 0.25, 0.25);
	BGStatsRandom(_RemainPoints, 5, &_Stats->Administration, &_Stats->Charisma, &_Stats->Intellegence, &_Stats->Intrigue, &_Stats->Piety, 0.3, 0.3, 0.15, 0.15, 0.1);
}

void BGSetAuthority(struct BigGuy* _Guy, float _Authority) {
	_Guy->Authority = _Authority;
	_Guy->IsDirty = 1;
}

void BGSetPrestige(struct BigGuy* _Guy, float _Prestige) {
	_Guy->Prestige = _Prestige;
	_Guy->IsDirty = 1;
}

void BigGuySetAction(struct BigGuy* _Guy, int _Action, void* _Data) {
	switch(_Action) {
	case BGACT_IMRPOVEREL:
		_Guy->Action.ActionFunc = BigGuyActionImproveRel;
		_Guy->Action.Data = _Data;
		break;
	default:
		_Guy->Action.ActionFunc = NULL;
		_Guy->Action.Data = NULL;
		break;
	}
}
