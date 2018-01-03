/*
 * File: Treaty.h
 * Author: David Brotz
 */

#include "Treaty.h"

#include "Government.h"
#include "World.h"

static const char* g_TreatyNames[TREATY_SIZE] = {
	"Alliance",
	"Ceasefire",
	"Tribute",
};

struct Treaty* CreateTreaty(unsigned int Type, unsigned int Length) {
	struct Treaty* Treaty = malloc(sizeof(struct Treaty));

	Treaty->Type = Type;
	Treaty->Date = g_GameWorld.Date;
	Treaty->End = DateAddMonths(Treaty->Date, Length);
	return Treaty;
}

void DestroyTreaty(struct Treaty* Treaty) {
	free(Treaty);
}

struct Treaty* FindTreaty(struct Treaties* List, void* Target) {
//	for(int i = 0; i < List->Treaties.Size; ++i) {
//		struct Treaty* Treaty = List->Treaties.Table[i];
//
//		if(Treaty->Target == Target) return Treaty;
//	}
	return NULL;
}

void CollectTribute(struct Government* Tribute, struct Government* Parent, uint16_t Payment) {
	struct BigGuy* TributeLeader = Tribute->Leader;
	struct BigGuy* ParentLeader = Parent->Leader;
}

struct Treaties* CreateTreatyList(struct Government* Owner, struct Government* Target) {
	struct Treaties* List = malloc(sizeof(struct Treaties));

	List->Owner = Owner;
	List->Target = Target;
	CtorArray(&List->Treaties, 4);
	return List;
}

void DestroyTreatyList(struct Treaties* List) {
	free(List);
}

void TreatyListThink(struct Treaties* List) {
	for(int i = 0; i < List->Treaties.Size; ++i) {
		struct Treaty* Treaty = List->Treaties.Table[i];

		if(DateCmp(g_GameWorld.Date, Treaty->End) >= 0) {
			DestroyTreaty(Treaty);
			ArrayRemove(&List->Treaties, i);	
		}
	}
}

const char* TreatyGetName(int Index) {
	if(Index < 0 || Index >= TREATY_SIZE) return NULL;
	return g_TreatyNames[Index];
}
