/**
 * File: Retinue.c
 * Author: David Brotz
 */

#include "Retinue.h"

#include "Person.h"
#include "BigGuy.h"
#include "Date.h"
#include "Location.h"
#include "World.h"
#include "Warband.h"
#include "Good.h"
#include "Culture.h"

#include "sys/Rule.h"

#include <stdlib.h>

struct RetNeed {
	const struct GoodBase* Base;
	uint16_t Quantity;
	uint8_t Priority;
};

void RetinueGetPos(const void* One, SDL_Point* Pos) {
	const struct Retinue* Retinue = One;
	
	Pos->x = Retinue->Home->Pos.x;
	Pos->y = Retinue->Home->Pos.y;
}

struct Retinue* CreateRetinue(struct BigGuy* Leader, struct GameWorld* World) {
	struct Retinue* Retinue = CreateObject(OBJECT_RETINUE);
	struct Settlement* Settlement = Leader->Person->Family->HomeLoc;
	SDL_Point Pos = Settlement->Pos;

	Retinue->Home = Settlement;
	Retinue->Leader = Leader;
	Retinue->IsRecruiting = 0;
	CtorArray(&Retinue->Warriors, 8);
	CtorArray(&Retinue->EquipNeed, 8);
	CtorArray(&Retinue->Children, 0);
#ifdef DEBUG
	if(World != NULL) IntInsert(&World->PersonRetinue, Leader->Person->Object.Id, Leader); 
#endif
	IntInsert(&World->PersonRetinue, Leader->Person->Object.Id, Leader); 
	QTInsertPoint(World->RetinueLoc, Retinue, &Pos);
	return Retinue;
}

void DestroyRetinue(struct Retinue* Retinue, struct GameWorld* World) {
	QTRemovePoint(World->RetinueLoc, &Retinue->Home->Pos, RetinueGetPos);
	DestroyObject(&Retinue->Object);
}

void RetinueThink(struct Retinue* Retinue) {
	if(NEWMONTH(g_GameWorld.Date) != 0) {
		for(int i = 0; i < Retinue->EquipNeed.Size; ++i) {

		}
	}
	if(Retinue->IsRecruiting != 0) {
		struct Settlement* Home = PersonHome(Retinue->Leader->Person);
		if(Home->FreeWarriors.Size <= 0)
			goto not_recruit;
		//if(BigGuySkillCheck(Retinue->Leader, BGSKILL_CHARISMA, SKILLCHECK_DEFAULT) == 0)
		//	goto not_recruit;
		RetinueAddWarrior(Retinue, Home->FreeWarriors.Front->Data);
		LnkLstPopFront(&Home->FreeWarriors);	
	}
	not_recruit:
	return;
}

static inline void InsertEquipNeed(struct Retinue* Retinue, struct WarRoleRule* Rule, const struct GoodBase* Base, uint16_t Quantity) {
	struct RetNeed* Need = NULL;

	for(int i = 0; i < Retinue->EquipNeed.Size; ++i) {
		Need = Retinue->EquipNeed.Table[i];

		if(Need->Base == Base) {
			Need->Quantity += Rule->Quantity - Quantity;
			return;
		}
	}
	Need = malloc(sizeof *Need);
	Need->Base = Base;
	Need->Quantity = Quantity;
	Need->Priority = 1;
	ArrayInsert_S(&Retinue->EquipNeed, Need);
}

void RetinueAddWarrior(struct Retinue* Retinue, struct Person* Warrior) {
	struct Family* Family = Warrior->Family;
	struct WarRoleRule* Rule = Family->HomeLoc->Culture->WarriorRules[WARROLE_LIGHTINF];
	const struct Culture* Culture = Warrior->Family->HomeLoc->Culture;
	uint64_t Found = 0;
	int Size = 0;

	if(RetinueIsWarrior(Retinue, Warrior) != 0 || Retinue->Leader->Person == Warrior)
		return;
	ArrayInsertSort_S(&Retinue->Warriors, (void*)Warrior, ObjectCmp);	
	IntInsert(&g_GameWorld.PersonRetinue, Warrior->Object.Id, Retinue);

	for(int i = 0; Rule[i].Rule != RULE_NONE; ++i, ++Size) {

		/**
		 * For every good check if it is in a rule, if it is then check if we have enough of that good.
		 */
		for(int j = 0; j < Family->Goods.Size; ++j) {
			struct Good* Good = Family->Goods.Table[j];
			struct ArmsBase* Base = (struct ArmsBase*) Good->Base;

			if((Base->Base.Category & GOOD_ARMS) == 0) continue;
			if(Rule[i].Type == Base->ArmsType) {
				Found = Found | (1 << i);
				switch(Rule[i].Rule) {
					case RULE_GREATERTHAN:
					case RULE_EQUAL:
						if(Rule[i].Quantity >= Good->Quantity) goto next_good;
						break;
				}
				/*
				 * Don't have enough of the good update the EquipNeed table to declare we need more of Good.
				 */
				 InsertEquipNeed(Retinue, &Rule[i], Good->Base, Good->Quantity);
			}
		}
		next_good:;
		Assert(sizeof(Found) * CHAR_BIT > i);
	}
	Found = ((1 << Size) - 1)	& (~Found);
	for(int i = ctz(Found); Found > 0; Found = Found & (~(1 << i)), i = ctz(Found)) {
		 InsertEquipNeed(Retinue, &Rule[i], (struct GoodBase*) Culture->Arms[Rule[i].Type], Rule[i].Quantity);
	}
	PushEvent(EVENT_JOINRETINUE, Warrior, Retinue);
}

void RetinueRemoveWarrior(struct Retinue* Retinue, struct Person* Warrior) {
	for(int i = 0; i < Retinue->Warriors.Size; ++i) {
		if(Retinue->Warriors.Table[i] == Warrior) {
			ArrayRemove(&Retinue->Warriors, i);
		}
	}
	PushEvent(EVENT_QUITRETINUE, Warrior, Retinue);
}

void RetinueAddChild(struct Retinue* Parent, struct Retinue* Child) {
	ArrayInsert_S(&Parent->Children, Child);
	Child->Parent = Parent;
}
