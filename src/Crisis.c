/**
 * Author: David Brotz
 * File: Crisis.h
 */

#include "Crisis.h"

#include "BigGuy.h"
#include "Person.h"
#include "Location.h"
#include "Government.h"

#include <string.h>

#define INCIDENT_CHANCE (269)

/*
CRIME_COMPENSATION,
CRIME_DUEL,
CRIME_OUTCAST,
CRIME_TRIAL,
CRIME_DEATH,
*/
const char* g_CrisisDemands[CRIME_SIZE] = {
    "[Offender.FirstName] to pay compensation"
    "allow a trail by combat"
    "outlaw [Offender.FirstName]"
    "put [Offender.FirstName] to a trail of law",
    "put [Offender.FirstName] to death"
};

/*const char* g_CrisisMissions[CRISIS_SIZE] = {
    NULL,
    "Crisis.1",
    NULL
};*/

struct Crisis* CreateCrisis(struct Person* Offender, struct Person* Defender, uint8_t Type) {
	struct Crisis* Crisis = malloc(sizeof(struct Crisis));

	Assert(Type < CRISIS_SIZE);
	Crisis->Type = Type;
	Crisis->Offender = Offender;
	Crisis->Defender = Defender;
	return Crisis;
}

void CrisisProcess(struct Crisis* Crisis, uint32_t Option) {
	struct Government* OffGov = Crisis->Offender->Family->HomeLoc->Government;
	struct Government* DefGov = Crisis->Offender->Family->HomeLoc->Government;
	if(OffGov != DefGov) {
		struct Relation* Rel = GetRelation(OffGov->Relations, DefGov);

		if(Rel == NULL) Rel = CreateRelation(OffGov, DefGov, &OffGov->Relations);
		ChangeRelation(Rel, Crisis->Type, -10, OPNLEN_MEDIUM, OPINION_AVERAGE);
	}

	if(IsBigGuy(Crisis->Offender) == true) {
		if(IsBigGuy(Crisis->Defender) == true) {
			change_rel:;
			struct BigGuy* OffBg = RBSearch(&g_GameWorld.BigGuys, Crisis->Offender);
			struct BigGuy* DefBg = RBSearch(&g_GameWorld.BigGuys, Crisis->Defender);
			struct Relation* Rel = GetRelation(OffBg->Relations, DefBg);
			
			if(Rel == NULL) Rel = CreateRelation(OffBg, DefBg, &OffBg->Relations);
			//TODO: The worse the defender feels about the outcome the worse their opniion should be.
			ChangeRelation(Rel, Crisis->Type, (Option - Crisis->Demand) * 5, OPNLEN_MEDIUM, OPINION_AVERAGE);	
		} else {
			//TODO: The worse the defender feels about the outcome the more likely they should become a big guy.
			if(Random(1, 10) < 3) {
				uint8_t BGStats[BGSKILL_SIZE];

				GenerateStats(Crisis->Offender->Family->Caste, &BGStats);
				CreateBigGuy(Crisis->Offender, &BGStats, BGMOT_REVENGE);
				goto change_rel;
			}
		}
	} else if(IsBigGuy(Crisis->Defender) == true) {
		//TODO: Add motivation to a big guy to get revenge.
	}
}

void GenerateCrisis(struct Settlement* Settlement) {
	return;
	uint16_t MaxCount = Random(0, (Settlement->AdultWomen + Settlement->AdultMen));
	uint8_t Count = 0;
	uint32_t TableSz = FrameSizeRemain() / sizeof(struct Settlement*);
	uint32_t SetSize = 0; //Number of settlements in range.
	struct Settlement** SettlementList = FrameAlloc(TableSz * sizeof(struct Settlement*));
	struct Crisis* Crisis = NULL;
	struct PersonSelector Selector = {0};
	struct Person** OffenderList = NULL;
	struct Person*** DefenderList = NULL;
	struct Person* Offender = NULL;
	struct Person* Defender = NULL;
	uint32_t OffenderSize = 0;
	uint32_t* DefenderSize = NULL;
	uint16_t DefSet = 0;

//#ifdef DEBUG
//return;
	//if(Settlement->Crisis.Size > 0) return;
	Count = 1;
//#else
	for(int i = 0; i < MaxCount; ++i)
		if(Random(0, INCIDENT_CHANCE) == 0) ++Count;
//:w
//#endif
	if(Count == 0) return;
	/**
	 * Create a PersonSelector and then fill it wil people from the Offender's settlement.
	 * Then find all nearby settlements and create an array for each found settlement to place their QueryPeople result into.
	 */
	InitPersonSelector(&Selector);
	SettlementsInRadius(&g_GameWorld, &Settlement->Pos, 20, SettlementList, &SetSize, TableSz);
	FrameReduce((TableSz * sizeof(SettlementList)) - (SetSize * sizeof(SettlementList)));
	OffenderList = QueryPeople((struct Person** const) Settlement->People.Table, Settlement->People.Size, &Selector, &OffenderSize);	
	DefenderList = alloca(sizeof(void*) * SetSize);
	DefenderSize = alloca(sizeof(uint16_t) * SetSize);
	memset(DefenderList, 0, sizeof(void*) * SetSize);
	Assert(SetSize < 256);
	do {
		/**
		 * Choose a random settlement as the settlement the defender is from.
		 * If a QueryPeople hasn't been run on that settlement generate one, then fill out the crisis struct
		 * and add it to the offender's settlement.
		 */
		DefSet = RandByte() % SetSize;
		if(DefenderList[DefSet] == NULL) {
			DefenderList[DefSet] = QueryPeople((struct Person** const) SettlementList[DefSet]->People.Table, SettlementList[DefSet]->People.Size, &Selector, &DefenderSize[DefSet]);
		}
		Offender = OffenderList[Random(0, Settlement->People.Size - 1)];
		Defender = DefenderList[DefSet][Random(0, SettlementList[DefSet]->People.Size - 1)];
		if(Offender == Defender)
			continue;
		Crisis = CreateCrisis(Offender, Defender, CRISIS_MURDER);//RandByte() % CRISIS_SIZE);
		ArrayInsert_S(&Settlement->Crisis, Crisis);
	} while(--Count > 0);
	//Remove all allocations from QueryPeople and from this function.
	FrameSet(SettlementList);
}

