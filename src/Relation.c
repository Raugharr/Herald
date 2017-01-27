/**
 * Author: David Brotz
 * File: BigGuyRelation.c
 */

#include "Relation.h"

#include "BigGuy.h"
#include "Trait.h"

#include "sys/Log.h"
#include "sys/Math.h"

#include "sys/Constraint.h"

#include <stdlib.h>

const char* g_OpinionActions[] = {
	"Theft",
	"Rumor",
	"Trait",
	"RaiseFyrd",
	"Attack"
	"Gift",
	"Lack of war",
	"Policy"
};

static inline void OpinionUpdate(struct Relation* Relation, int Mod) {
	Relation->Modifier += Mod;
	Relation->Relation = Fuzify(g_OpinionMods, Relation->Modifier);
}

struct Relation* CreateRelation(void* Owner, void* Target, struct Relation** Next) {
	struct Relation* Relation = malloc(sizeof(struct Relation));

	Relation->Relation = BGREL_NEUTURAL;
	Relation->Modifier = 0;
	Relation->Next = *Next;
	*Next= Relation;
	Relation->Opinions = NULL;
	Relation->Target = Target;
	return Relation;
}

struct Opinion* CreateOpinion(struct Relation* Relation, int Action, int Modifier, int RelLen, int Strength) {
	struct Opinion* Opinion = (struct Opinion*) malloc(sizeof(struct Opinion));

	Opinion->Action = Action;
	Opinion->RelMod = Modifier;
	Opinion->RelLength = RelLen;
	Opinion->RelStrength = Strength;
	Opinion->MonthTick = 0;
	Opinion->Next = Relation->Opinions;
	Relation->Opinions = Opinion;
	OpinionUpdate(Relation, Modifier);
	return Opinion;
}

void AddOpinion(void* Owner, void*  Target, int Action, int Modifier, int RelLen, int Strength, struct Relation** Relation) {
	if(Modifier == 0)
		return;
	for(struct Relation* Itr = *Relation; Itr != NULL; Itr = Itr->Next) {
		if(Target != Itr->Target)
			continue;
		ChangeRelation(Itr, Action, Modifier, RelLen, Strength);
		return;
	}
	CreateOpinion(CreateRelation(Owner, Target, Relation), Action, Modifier, RelLen, Strength);
}

void ChangeRelation(struct Relation* Relation, int Action, int Modifier, int RelLen, int Strength) {
	Assert(Relation != NULL);

	for(struct Opinion* Opinion = Relation->Opinions; Opinion != NULL; Opinion = Opinion->Next) {
		if(Opinion->Action != Action || Opinion->RelStrength != Strength)
			continue;
		Opinion->RelMod += Modifier;
		OpinionUpdate(Relation, Modifier);
		return;
	}
	CreateOpinion(Relation, Action, Modifier, RelLen, Strength);
}

static inline void BigGuyRelationTickOpinion(struct Opinion* Opinion, int Mod) {
//	Opinion->RelMod = Opinion->RelMod + ((Opinion->RelMod > 0) ? (-Mod) : (Mod));
	if(Opinion->RelMod < 0) {
		Opinion->RelMod += Mod;
		if(Opinion->RelMod > 0) {
			Opinion->RelMod = 0;
		}
	} else if(Opinion->RelMod > 0) {
		Opinion->RelMod -= Mod;
		if(Opinion->RelMod < 0) {
			Opinion->RelMod = 0;
		}
	}
}

void RelUpdate(struct Relation* Relation) {
	struct Opinion* Opinion = Relation->Opinions;
	int Modifier = 0;

	if(DAY(g_GameWorld.Date) != 0)
		return;
	for(Opinion = Relation->Opinions; Opinion != NULL; Opinion = Opinion->Next) {
		if(Opinion->RelLength != OPNLEN_FOREVER && Opinion->RelStrength != OPINION_STATIC) {
			switch(Opinion->MonthTick) {
				case OPNLEN_SMALL:
					BigGuyRelationTickOpinion(Opinion, 2);
					break;
				case OPNLEN_MEDIUM:
					BigGuyRelationTickOpinion(Opinion, 1);
					break;
				case OPNLEN_LARGE:
					if(Opinion->MonthTick == 0) {
						++Opinion->MonthTick;
						break;
					}
					Opinion->MonthTick = 0;
					BigGuyRelationTickOpinion(Opinion, 2);
					break;
			}
		}
		Modifier += Opinion->RelMod;
	}
	Relation->Modifier = Modifier;
	Relation->Relation = Fuzify(g_OpinionMods, Relation->Modifier);
}

struct Relation* GetRelation(struct Relation* Relation, const void* Target) {
	while(Relation != NULL) {
		if(Relation->Target== Target)
			return Relation;
		Relation = Relation->Next;
	}
	return NULL;
}


