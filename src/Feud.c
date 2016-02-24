/*
 * File: Feud.c
 * Author: David Brotz
 */

#include "Feud.h"

#include "Herald.h"
#include "Family.h"
#include "Person.h"
#include "BigGuy.h"
#include "World.h"
#include "Good.h"

#include "sys/Array.h"
#include "sys/Math.h"
#include "sys/Event.h"

#include <stdlib.h>

#define FEUD_INCR (1)
#define FEUD_THINKTIME (180)

void FeudAddBigGuy(struct Feud* _Feud, const struct Family* _Family) {
	struct BigGuy* _Guy = NULL;

	for(int i = 0; i < FAMILY_PEOPLESZ; ++i)
		if(_Family->People[i] != NULL && (_Guy = RBSearch(&g_GameWorld.BigGuys, _Family->People[i])) != NULL)
			BigGuyAddFeud(_Guy, _Feud);
}

struct Feud* CreateFeud(struct BigGuy* _SideOne, struct BigGuy* _SideTwo) {
	struct Feud* _Feud = (struct Feud*) malloc(sizeof(struct Feud));

	CreateObject((struct Object*)_Feud, OBJECT_FEUD, (ObjectThink) FeudThink);
	_Feud->Level = 0;
	_Feud->Side[0].Size = 0;
	_Feud->Side[0].Front = NULL;
	_Feud->Side[0].Back = NULL;

	_Feud->Side[1].Size = 0;
	_Feud->Side[1].Front = NULL;
	_Feud->Side[1].Back = NULL;

	LnkLstPushBack(&_Feud->Side[0], _SideOne->Person->Family);
	LnkLstPushBack(&_Feud->Side[1], _SideTwo->Person->Family);
	_Feud->Leader[0] = _SideOne;
	_Feud->Leader[1] = _SideTwo;
	FeudAddBigGuy(_Feud, _SideOne->Person->Family);
	FeudAddBigGuy(_Feud, _SideTwo->Person->Family);
	PushEvent(EVENT_FEUD, _SideOne, _SideTwo);
	return _Feud;
}

void DestroyFeud(struct Feud* _Feud) {
	LnkLstClear(&_Feud->Side[0]);
	LnkLstClear(&_Feud->Side[1]);
	DestroyObject((struct Object*)_Feud);
	free(_Feud);
}

void FeudAddFamily(struct Feud* _Feud, struct Family* _Family, int _Side) {
	if(_Side != 0 || _Side != 1)
		return;
	LnkLstPushBack(&_Feud->Side[_Side], _Family);
	FeudAddBigGuy(_Feud, _Family);
}

void FeudThink(struct Feud* _Feud) {
	int _Side = 0;
	int _Quantity = 0;
	int _Index = 0;
	struct Family* _Family = NULL;
	struct Family* _Taker = NULL;
	int _GoodsTaken = 0;
	int _AnimalsTaken = 0;
	int _PeopleKilled = 0;
	struct BigGuyRelation* _Relation = NULL;
	struct Good* _TakenGood = NULL;

	if(_Feud->LastThink + (FEUD_THINKTIME / _Feud->Level) <= g_GameWorld.Tick)
		return;
	if(((_Relation = BigGuyGetRelation(_Feud->Leader[0], _Feud->Leader[1])) == NULL || _Relation->Relation > BGREL_DISLIKE)
			&& ((_Relation = BigGuyGetRelation(_Feud->Leader[1], _Feud->Leader[0])) == NULL || _Relation->Relation > BGREL_DISLIKE)) {
		DestroyFeud(_Feud);
		return;
	}

	_Side = Random(0, 1);
	_Family = (struct Family*) LnkLstRandom(&_Feud->Side[_Side]);
	_Taker = (struct Family*) LnkLstRandom(&_Feud->Side[!_Side]);
	_GoodsTaken = _Feud->Level;
	_AnimalsTaken = _Feud->Level / 3;
	_PeopleKilled = _Feud->Level / 5;

	if(_AnimalsTaken > _Family->Animals->Size)
		_AnimalsTaken = _Family->Animals->Size;
	if(_GoodsTaken > _Family->Goods->Size)
		_GoodsTaken = _Family->Goods->Size;
	while(_GoodsTaken > 0) {
		_Index = Random(0, _Family->Goods->Size - 1);
		//_Quantity = Random(1, ((struct Good*)_Family->Goods->Table[_Index])->Quantity);
		_TakenGood = FamilyTakeGood(_Family, _Index, _Quantity);
		FamilyGetGood(_Taker, _TakenGood, _TakenGood->Quantity);
		//_GoodsTaken = _GoodsTaken - _Quantity;
		--_GoodsTaken;
	}

	while(_AnimalsTaken > 0) {
		FamilyTakeAnimal(_Family, Random(0, _Family->Animals->Size - 1));
		--_AnimalsTaken;
	}

	while(_PeopleKilled > 0) {
		--_PeopleKilled;
	}
	_Feud->Level = _Feud->Level + (FEUD_INCR * (_Feud->Level + 1));
	_Feud->LastThink = g_GameWorld.Tick;
}
