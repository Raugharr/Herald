/**
 * Author: David Brotz
 * File: Faction.h
 */

#include "Faction.h"

#include "Person.h"
#include "Family.h"
#include "BigGuy.h"
#include "Government.h"
#include "World.h"

#include "sys/LuaCore.h"

#include "sys/Coroutine.h"

#include <stdlib.h>

/**
 * The composition of each faction when it is created.
 * For example if only the noble and peasant factions exist then for the farmer caste the nobles will get 3 of every 8 farmers
 * and the peasant faction will get 5 of every 8 farmers on average.
 */
static uint8_t g_FactionWeights[FACTION_IDSIZE * CASTE_SIZE] = {
	3, 3, 2, 4, 1, 7, 7,
	5, 5, 2, 2, 1, 3, 3,
	3, 3, 1, 2, 9, 1, 1,
	1, 1, 7, 4, 1, 1, 1
};

const char* g_FactionGoalNames[FACTION_GSIZE] = {
	"None",
	"Lower Taxes",
	"Raise Taxes",	
	"Change Caste",
	"Introduce Policy",
	"Remove Policy"
};

const char* g_FactionNames[FACTION_GSIZE] = {
	"Noble",
	"Peasant",
	"Religion",
	"Merchant"
};

struct Faction* CtorFaction(struct Faction* Faction, struct Settlement* Settlement) {
	Faction->Settlement = Settlement;
	for(int i = 0; i < FACTION_IDSIZE; ++i) {
		Faction->Leader[i] = NULL;
		CtorArray(&Faction->Bosses[i], 8);
		CtorArray(&Faction->Mob[i], 32);
		Faction->Power[i] = 0;
		Faction->PowerGain[i] = 6;
		Faction->LastGoal[i] = 0;
		Faction->PolicyInfluence[i] = -1;
		Faction->OpposePolicy[i] = false;
		for(int j = 0; j < CASTE_SIZE; ++j) {
			Faction->CasteCount[FactionCasteIdx(i, j)] = 0;
			Faction->CastePower[j] = 6;
			Faction->FactionWeight[FactionCasteIdx(i, j)] = g_FactionWeights[FactionCasteIdx(i, j)];
		}
	}
	Faction->ActiveMask = (1 << FACTION_IDPEASANT) | (1 << FACTION_IDNOBLE);
	Faction->Coro = -1;
	return Faction;
}

void DestroyFaction(struct Faction* Faction) {
	free(Faction);
}

/*void FactionOnLeaderDeath(struct Faction* Faction, uint8_t Ideology) {
	if(Faction->Bosses[Ideology].Size > 0) {
		Faction->Leader[Ideology] = Faction->Bosses[Ideology].Table[0];
		ArrayRemove(&Faction->Bosses[Ideology], 0);
	} else {
		DestroyFaction(Faction);
	}
}*/

void FactionAddBoss(struct Faction* Faction, uint8_t Ideology, struct BigGuy* Boss) {
	Faction->PowerGain[Ideology] += Faction->CastePower[Boss->Person->Family->Caste];
	ArrayInsert_S(&Faction->Bosses[Ideology], Boss);
}

void FactionAddPerson(struct Faction* Faction, uint8_t Ideology, struct Person* Person) {
	if((Faction->ActiveMask & (1 << Ideology)) != (1 << Ideology))
		return;
	Faction->PowerGain[Ideology] += Faction->CastePower[Person->Family->Caste];
	Faction->CasteCount[FactionCasteIdx(Ideology, Person->Family->Caste)]++;
	ArrayInsert_S(&Faction->Mob[Ideology], Person);
}

void FactionRemovePerson(struct Faction* Faction, uint8_t Ideology, struct Person* Person) {
	struct Person* Temp = NULL;

	if((Faction->ActiveMask & (1 << Ideology)) != (1 << Ideology))
		return;
	for(int i = 0; i < Faction->Mob[Ideology].Size; ++i) {
		Temp = Faction->Mob[Ideology].Table[i];
		if(Temp == Person) {
			ArrayRemove(&Faction->Mob[Ideology], i);
			return;
		}
	}
	Faction->CasteCount[FactionCasteIdx(Ideology, Person->Family->Caste)]--;
	Faction->PowerGain[Ideology] -= Faction->CastePower[Person->Family->Caste];
}

void FactionGoalCoro(struct Faction* Faction, int Ideology, int Goal, int Data1, int Data2) {
	struct FactionOpCode GoalOp;
	
	GoalOp.Type = Goal; 
	switch(Goal) {
		case FACTION_CHCASTE:
			GoalOp.Caste.Caste = Data1;
			GoalOp.Caste.FromCaste = Data2;
		break;
	}
	for(int i = 0; i < FACTION_IDSIZE; ++i) {
		if(FactionIsActive(Faction, i) == true && Faction->Leader[i] != g_GameWorld.Player) {
			FactionPickSide(Faction, i, true);
			FactionBet(Faction, i, Random(0, Faction->Power[i]));
		}
	}
	FactionPickSide(Faction, Ideology, false);
	for(int i = 0; i < FACTION_IDSIZE; ++i) {
		if(FactionIsActive(Faction, i) == false)
			continue;
		while(Faction->FactionBet[i] == -1)	CoYield();
	}
	Faction->Coro = -1;
	EventFactionGoalEnd(Faction, Goal, g_LuaState);
	Faction->LastGoal[Ideology] = g_GameWorld.Date;
	FactionPassGoal(Faction, Ideology, &GoalOp);
}

bool FactionBet(struct Faction* Faction, uint8_t Ideology, uint16_t Bet) {
	if(Bet > Faction->Power[Ideology])
		return false;
	Faction->Power[Ideology] -= Bet;
	Faction->FactionBet[Ideology] = Bet;
	if(Faction->Coro == -1) {
		Faction->Coro = CoSpawn(FactionGoalCoro, 3, Faction, Ideology, Bet);
	}
	return true;
}

void FactionSetGoal(struct Faction* Faction, uint8_t Ideology, uint8_t Goal, uint8_t Data1, uint8_t Data2) {
	if(Faction->Goal != FACTION_GNONE)
		return;
	Faction->Goal = Goal;
	for(int i = 0; i < FACTION_IDSIZE; ++i) {
		Faction->FactionBet[i] = -1;
	}
	if(Faction->Coro == -1) {
		Faction->Coro = CoSpawn(FactionGoalCoro, 5, Faction, ((int)Ideology), ((int)Goal), (int)Data1, (int)Data2);
		CoResume(Faction->Coro);
	}
}

bool FactionValGoal(struct Faction* Faction, uint8_t Ideology, uint8_t Goal) {
	switch(Goal) {
		case FACTION_GLTAXES:
			return (Faction->Settlement->Government->TaxRate == TAX_MIN);
		case FACTION_GRTAXES:
			return (Faction->Settlement->Government->TaxRate >= TAX_MAX);
		case FACTION_CHCASTE:
		case FACTION_GLPOLICY:
		case FACTION_GRPOLICY:
			return true;
	}
	return false;
}

void FactionPassGoal(struct Faction* Faction, uint8_t Ideology, struct FactionOpCode* Goal) {
	switch(Goal->Type) {
		case FACTION_CHCASTE:
			Faction->CastePower[Goal->Caste.Caste] += 1;
			Faction->CastePower[Goal->Caste.FromCaste] -= 1;
			break;
		case FACTION_GRTAXES:
			Faction->Settlement->Government->TaxRate++;
			break;
		case FACTION_GLTAXES:
			Faction->Settlement->Government->TaxRate--;
			break;
	}
	Faction->Goal = FACTION_GNONE;
}

uint32_t FactionInfluenceCost(struct Faction* Faction, uint8_t Ideology) {
	uint32_t FactionMem = 0;

	for(int i = 0; i < FACTION_IDSIZE; ++i) {
		if(FactionIsActive(Faction, i) == false)
			continue;
		FactionMem += Faction->Bosses[i].Size + Faction->Mob[i].Size + 1;
	}
	return (Faction->Bosses[Ideology].Size + Faction->Mob[Ideology].Size + 1) * 100 / FactionMem;
}
