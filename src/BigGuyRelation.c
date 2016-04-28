/**
 * Author: David Brotz
 * File: BigGuyRelation.c
 */

#include "BigGuyRelation.h"

#include "BigGuy.h"
#include "Trait.h"

#include "sys/Constraint.h"

#include <stdlib.h>

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
				BigGuyAddRelation(_Guy, _Relation, ACTTYPE_TRAIT, -(BIGGUY_TRAITREL)); 
			} else if(TraitLikes(_Guy->Traits[i], _Actor->Traits[j])) {
				BigGuyAddRelation(_Guy, _Relation, ACTTYPE_TRAIT, BIGGUY_TRAITREL); 
			}
		}
	}
	return _Relation;
}

struct BigGuyOpinion* CreateBigGuyOpinion(struct BigGuyRelation* _Relation, int _Action, int _Modifier) {
	struct BigGuyOpinion* _Opinion = (struct BigGuyOpinion*) malloc(sizeof(struct BigGuyOpinion));

	_Opinion->Action = _Action;
	_Opinion->RelMod = _Modifier;
	_Opinion->Next = _Relation->Opinions;
	_Relation->Opinions = _Opinion;
	return _Opinion;
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
	_Relation->Modifier = _Relation->Modifier + (((float)_Modifier) * BigGuyOpinionMod(_Guy, _Relation->Person));
	_Relation->Relation = Fuzify(g_OpinionMods, _Relation->Modifier);
}

void BigGuyChangeOpinion(struct BigGuy* _Guy, struct BigGuy* _Target, int _Action, int _Modifier) {
	struct BigGuyRelation* _Relation = _Guy->Relations;
	struct BigGuyOpinion* _Opinion = NULL;

	while(_Relation != NULL) {
		_Opinion = _Relation->Opinions;
		if(_Target != _Relation->Person)
			goto rel_end;
		while(_Opinion != NULL) {
			if(_Opinion->Action != _Action)
				goto opin_end;
			_Opinion->RelMod += _Modifier;
			opin_end:
			_Opinion = _Opinion->Next;
		}
		rel_end:
		_Relation = _Relation->Next;
	}
	_Relation = CreateBigGuyRelation(_Guy, _Target);
	CreateBigGuyOpinion(_Relation, _Action, _Modifier);
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

