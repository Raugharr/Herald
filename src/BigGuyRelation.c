/**
 * Author: David Brotz
 * File: BigGuyRelation.c
 */

#include "BigGuyRelation.h"

#include "BigGuy.h"
#include "Trait.h"

#include "sys/Log.h"
#include "sys/Math.h"

#include "sys/Constraint.h"

#include <stdlib.h>

const char* g_BigGuyOpinionActions[] = {
	"Theft",
	"Rumor",
	"Trait",
	"RaiseFyrd",
	"Attack"
	"Gift",
	"Lack of war",
	"Policy"
};

static inline void BigGuyOpinionUpdate(struct BigGuyRelation* _Relation, int _Mod) {
	_Relation->Modifier += _Mod;
	_Relation->Relation = Fuzify(g_OpinionMods, _Relation->Modifier);
}

struct BigGuyRelation* CreateBigGuyRelation(struct BigGuy* _Guy, struct BigGuy* _Actor) {
	struct BigGuyRelation* _Relation = (struct BigGuyRelation*) malloc(sizeof(struct BigGuyRelation));

	_Relation->Relation = BGREL_NEUTURAL;
	_Relation->Modifier = 0;
	_Relation->Next = _Guy->Relations;
	_Guy->Relations = _Relation;
	_Relation->Opinions = NULL;
	_Relation->Person = _Actor;

	for(int i = 0; _Guy->Traits[i] != NULL; ++i) {
		for(int j = 0; _Actor->Traits[j] != NULL; ++j) {
			if(TraitDislikes(_Guy->Traits[i], _Actor->Traits[j]) != 0) {
				BigGuyChangeRelation(_Relation, ACTTYPE_TRAIT, -(BIGGUY_TRAITREL), OPNLEN_FOREVER, OPINION_STATIC);
			} else if(TraitLikes(_Guy->Traits[i], _Actor->Traits[j])) {
				BigGuyChangeRelation(_Relation, ACTTYPE_TRAIT, BIGGUY_TRAITREL, OPNLEN_FOREVER, OPINION_STATIC);
			}
		}
	}
	return _Relation;
}

struct BigGuyOpinion* CreateBigGuyOpinion(struct BigGuyRelation* _Relation, int _Action, int _Modifier, int _RelLen, int _Strength) {
	struct BigGuyOpinion* _Opinion = (struct BigGuyOpinion*) malloc(sizeof(struct BigGuyOpinion));

	_Opinion->Action = _Action;
	_Opinion->RelMod = _Modifier;
	_Opinion->RelLength = _RelLen;
	_Opinion->RelStrength = _Strength;
	_Opinion->MonthTick = 0;
	_Opinion->Next = _Relation->Opinions;
	_Relation->Opinions = _Opinion;
	BigGuyOpinionUpdate(_Relation, _Modifier);
	return _Opinion;
}

void BigGuyAddOpinion(struct BigGuy* _Guy, struct BigGuy* _Target, int _Action, int _Modifier, int _RelLen, int _Strength) {
	if(_Modifier == 0)
		return;
	for(struct BigGuyRelation* _Relation = _Guy->Relations; _Relation != NULL; _Relation = _Relation->Next) {
		if(_Target != _Relation->Person)
			continue;
		BigGuyChangeRelation(_Relation, _Action, _Modifier, _RelLen, _Strength);
		return;
	}
	CreateBigGuyOpinion(CreateBigGuyRelation(_Guy, _Target), _Action, _Modifier, _RelLen, _Strength);
}

void BigGuyChangeRelation(struct BigGuyRelation* _Relation, int _Action, int _Modifier, int _RelLen, int _Strength) {
	Assert(_Relation != NULL);

	for(struct BigGuyOpinion* _Opinion = _Relation->Opinions; _Opinion != NULL; _Opinion = _Opinion->Next) {
		if(_Opinion->Action != _Action || _Opinion->RelStrength != _Strength)
			continue;
		_Opinion->RelMod += _Modifier;
		BigGuyOpinionUpdate(_Relation, _Modifier);
		return;
	}
	CreateBigGuyOpinion(_Relation, _Action, _Modifier, _RelLen, _Strength);
}

static inline void BigGuyRelationTickOpinion(struct BigGuyOpinion* _Opinion, int _Mod) {
//	_Opinion->RelMod = _Opinion->RelMod + ((_Opinion->RelMod > 0) ? (-_Mod) : (_Mod));
	if(_Opinion->RelMod < 0) {
		_Opinion->RelMod += _Mod;
		if(_Opinion->RelMod > 0) {
			_Opinion->RelMod = 0;
		}
	} else if(_Opinion->RelMod > 0) {
		_Opinion->RelMod -= _Mod;
		if(_Opinion->RelMod < 0) {
			_Opinion->RelMod = 0;
		}
	}
}

void BigGuyRelationUpdate(struct BigGuyRelation* _Relation) {
	struct BigGuyOpinion* _Opinion = _Relation->Opinions;
	int _Modifier = 0;

	if(DAY(g_GameWorld.Date) != 0)
		return;
	for(_Opinion = _Relation->Opinions; _Opinion != NULL; _Opinion = _Opinion->Next) {
		if(_Opinion->RelLength != OPNLEN_FOREVER && _Opinion->RelStrength != OPINION_STATIC) {
			switch(_Opinion->MonthTick) {
				case OPNLEN_SMALL:
					BigGuyRelationTickOpinion(_Opinion, 2);
					break;
				case OPNLEN_MEDIUM:
					BigGuyRelationTickOpinion(_Opinion, 1);
					break;
				case OPNLEN_LARGE:
					if(_Opinion->MonthTick == 0) {
						++_Opinion->MonthTick;
						break;
					}
					_Opinion->MonthTick = 0;
					BigGuyRelationTickOpinion(_Opinion, 2);
					break;
			}
		}
		_Modifier += _Opinion->RelMod;
	}
	_Relation->Modifier = _Modifier;
	_Relation->Relation = Fuzify(g_OpinionMods, _Relation->Modifier);
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

