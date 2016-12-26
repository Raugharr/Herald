/*
 * File: Battle.c
 * Author: David Brotz
 */

#include "Battle.h"

#include "Herald.h"
#include "Person.h"
#include "BigGuy.h"
#include "Government.h"
#include "Family.h"
#include "Location.h"

#include "sys/LinkedList.h"
#include "sys/Event.h"
#include "sys/Math.h"

#include <stdlib.h>

#define BATTLE_FIRSTFRONT (BATTLE_MAXFRONTS / 2)
#define FRONT_STARTRANGE (0)
#define BATTLE_SAFERANGE (7)
#define BATTLE_TICKS (2)
#define BATTLE_ATTACKER (0)
#define BATTLE_DEFENDER (1)

void FrontStats(uint8_t (*Stats)[WARSTAT_SIZE], struct Array* Front) {
	uint8_t TempStats[WARSTAT_SIZE] = {0};

	if(Front->Size < 1)
		return;
	for(int i = 0; i < Front->Size; ++i) {
		struct Warband* Warband = Front->Table[i];

		for(int k = 0; k < WARSTAT_SIZE; ++k) {
			TempStats[k] = Warband->Stats[k];
		}
	}
	for(int i = 0; i < WARSTAT_SIZE; ++i) {
		(*Stats)[i] = TempStats[i] / Front->Size;
	}
}

struct Battle* CreateBattle(struct Army* Attacker, struct Army* Defender) {
	struct Battle* Battle = NULL;
	struct Battle** List = (struct Battle**) SubTimeGetList(SUBTIME_BATTLE);

	if(Attacker->WarbandCt <= 0 || Attacker->InBattle != 0) {
		return NULL;
	}

	if(Defender->WarbandCt <= 0 || Attacker->InBattle != 0) {
		return NULL;
	}

	Battle = (struct Battle*) malloc(sizeof(struct Battle));
	Attacker->InBattle = true;
	Defender->InBattle = true;
	Battle->Side[BATTLE_ATTACKER].Army = Attacker;
	Battle->Side[BATTLE_ATTACKER].StartingSize = ArmyGetSize(Battle->Side[BATTLE_ATTACKER].Army);

	Battle->Side[BATTLE_DEFENDER].Army = Defender;
	Battle->Side[BATTLE_DEFENDER].StartingSize = ArmyGetSize(Battle->Side[BATTLE_DEFENDER].Army);
	for(int i = 0; i < BATTLE_MAXFRONTS; ++i) {
		CtorArray(&Battle->Side[BATTLE_ATTACKER].FrontWarbands[i], Attacker->WarbandCt);
		CtorArray(&Battle->Side[BATTLE_DEFENDER].FrontWarbands[i], Defender->WarbandCt);
		Battle->Side[BATTLE_ATTACKER].FrontSize[i] = 0;
		Battle->Side[BATTLE_DEFENDER].FrontSize[i] = 0;
	}
	for(struct Warband* Warband = Attacker->Warbands; Warband != NULL; Warband = Warband->Next) {
		ArrayInsert(&Battle->Side[BATTLE_ATTACKER].FrontWarbands[BATFLANK_MID], Warband);
		Battle->Side[BATTLE_ATTACKER].FrontSize[BATFLANK_MID] += Warband->Warriors.Size;
	}
	for(struct Warband* Warband = Defender->Warbands; Warband != NULL; Warband = Warband->Next) {
		ArrayInsert(&Battle->Side[BATTLE_DEFENDER].FrontWarbands[BATFLANK_MID], Warband);
		Battle->Side[BATTLE_DEFENDER].FrontSize[BATFLANK_MID] += Warband->Warriors.Size;
	}
	for(int i = 0; i < BATTLE_SIDES; ++i) {
		InitUnitStats((uint8_t(*)[WARSTAT_SIZE])&Battle->Side[BATTLE_ATTACKER].Stats[WARSTAT_SIZE * i]);
		ArmyUpdateStats(Battle->Side[BATTLE_ATTACKER].Army);
		FrontStats((uint8_t(*)[WARSTAT_SIZE])&Battle->Side[BATTLE_ATTACKER].Stats[WARSTAT_SIZE * i], &Battle->Side[BATTLE_ATTACKER].FrontWarbands[i]);

		InitUnitStats((uint8_t(*)[WARSTAT_SIZE])&Battle->Side[BATTLE_DEFENDER].Stats[i]);
		ArmyUpdateStats(Battle->Side[BATTLE_DEFENDER].Army);
		FrontStats((uint8_t(*)[WARSTAT_SIZE])&Battle->Side[BATTLE_DEFENDER].Stats[WARSTAT_SIZE * i], &Battle->Side[BATTLE_DEFENDER].FrontWarbands[i]);
	}
	ILL_CREATE(*List, Battle);
	return Battle;
}


void DestroyBattle(struct Battle* Battle) {
	struct Battle** List = (struct Battle**) SubTimeGetList(SUBTIME_BATTLE);

	ILL_DESTROY(*List, Battle);
	Battle->Side[BATTLE_ATTACKER].Army->InBattle = false;
	Battle->Side[BATTLE_DEFENDER].Army->InBattle = false;
	free(Battle);
}

//Array of struct Warband*
//Returns the size of all the warbands soldiers.
uint16_t CountFrontSize(const struct Array* Front) {
	uint16_t Size = 0;
	struct Warband* Warband = NULL;

	for(int i = 0; i < Front->Size; ++i) {
		Warband = Front->Table[i];
		Size += Warband->Warriors.Size;
	}
	return Size;
}

void BattleDistPrestige(struct BattleSide* Side, int TotalPrestige) {
	struct Army* Army = Side->Army;
	struct Warband* Warband = Army->Warbands;
	struct Warrior* Warrior = NULL;
	struct LnkLst_Node* Itr = NULL;
	struct LinkedList List = LinkedList(); 
	struct Warrior* Ptr = NULL;
	struct BigGuy* Guy = NULL;
	int CasteWarriorCt = 0;

	while(Warband != NULL) { 
		for(int i = 0; i < Warband->Warriors.Size; ++i) {
			Warrior = Warband->Warriors.Table[i];
			if(PERSON_CASTE(Warrior->Person) != CASTE_NOBLE) {
				continue;
			}
			Ptr = Warrior;
			LnkLstPushBack(&List, Ptr);
			++CasteWarriorCt;
		}
		Warband = Warband->Next;
	}
	Itr = List.Front;
	while(Itr != NULL) {
		if((Guy = RBSearch(&g_GameWorld.BigGuys, ((struct Warrior*)Itr->Data)->Person)) != NULL) {
		} else 
		Itr = Itr->Next;
	}
	LnkLstClear(&List);
}

void BattleEnd(int Victor, struct Battle* Battle) {
	int AttkSize = ArmyGetSize(Battle->Side[BATTLE_ATTACKER].Army);
	int DefSize = ArmyGetSize(Battle->Side[BATTLE_DEFENDER].Army);
	int DefPrestige = (((float)DefSize) / Battle->Side[BATTLE_DEFENDER].StartingSize) / (((float)AttkSize) / Battle->Side[BATTLE_ATTACKER].StartingSize);
	int AttkPrestige = (((float)AttkSize) / Battle->Side[BATTLE_ATTACKER].StartingSize) / (((float)DefSize) / Battle->Side[BATTLE_DEFENDER].StartingSize);

	BattleDistPrestige(&Battle->Side[BATTLE_ATTACKER], AttkPrestige);
	BattleDistPrestige(&Battle->Side[BATTLE_DEFENDER], DefPrestige);
	PushEvent(EVENT_BATTLE, Battle, NULL);
}


uint16_t BattleDamage(uint8_t HitChance, uint8_t WoundChance, uint8_t BlockChance, uint16_t Rolls) {
	uint16_t Hits = 0;
	uint16_t Cas = 0;

	for(int i = 0; i < Rolls; ++i) {
		if(Random(1, 100) <= HitChance)
			++Hits;
	}
	for(int i = 0; i < Hits; ++i) {
		uint16_t OffRoll = Random(1, 100) + WoundChance;
		uint16_t DefRoll = Random(1, 100) + BlockChance;

		if(OffRoll > DefRoll)
			++Cas;
	}
	return Cas;	
}

void RemoveCasualties(struct Array* Front, uint16_t Amount, uint16_t ArmySize) {
	struct Warrior* Warrior = NULL;
	struct Warband* Warband = NULL;
	uint16_t Rand = 0;
	struct Warband* RandList[Front->Size];
	void* Temp = NULL;
	uint16_t Size = Front->Size;
	int* WarbandDeaths = alloca(sizeof(int) * Front->Size);
	double DeathPercent[Front->Size];
	
	if(ArmySize < 1)
		return;
	for(int i = 0; i < Front->Size; ++i) {
		RandList[i] = Front->Table[i];
		DeathPercent[i] = ((struct Warband*)Front->Table[i])->Warriors.Size / ArmySize;
	}
	while(Size > 1) {
		Rand = Random(0, Size - 1);
		Temp = Front->Table[Rand];
		Front->Table[Rand] = Front->Table[Size - 1];
		Front->Table[Size - 1] = Temp;
		--Size;
	}
	RandTable(DeathPercent, &WarbandDeaths, Front->Size, Amount);
	//For every warband.
	for(int i = 0; i < Front->Size; ++i) {
		Warband = RandList[i];
		//Pick a random amount of people from the current warband to die.
		Amount -= WarbandDeaths[i];
		//Kill number of people.
		for(int k = 0; k < WarbandDeaths[i]; ++k) {
			if(Warband->Warriors.Size < 1)
				goto end;
			Rand = Random(0, Warband->Warriors.Size - 1);
			Warrior = Warband->Warriors.Table[Rand]; 
			Warband->Warriors.Table[Rand] = Warband->Warriors.Table[Warband->Warriors.Size - 1];
			--Warband->Warriors.Size;
			PersonDeath(Warrior->Person);
		}
		end:
		;
	//Front->Table[Rand] = Front->Table[Front->Size - 1];
	}
}

void BattleThink(struct Battle* Battle) {
	uint16_t AttkCas = 0;
	uint16_t DefCas = 0;

	for(int Tick = 0; Tick < BATTLE_TICKS; ++Tick) {
		for(int i = 0; i < BATFLANK_SIZE; ++i) {
			uint8_t StatOff = i * WARSTAT_SIZE;
			DefCas = BattleDamage(Battle->Side[BATTLE_ATTACKER].Stats[StatOff + WARSTAT_COMBAT], Battle->Side[BATTLE_ATTACKER].Stats[StatOff + WARSTAT_COMBAT],
				Battle->Side[BATTLE_DEFENDER].Stats[StatOff +  WARSTAT_DEFENSE], CountFrontSize(&Battle->Side[BATTLE_ATTACKER].FrontWarbands[i]));

			AttkCas = BattleDamage(Battle->Side[BATTLE_DEFENDER].Stats[StatOff + WARSTAT_COMBAT], Battle->Side[BATTLE_DEFENDER].Stats[StatOff + WARSTAT_COMBAT],
				Battle->Side[BATTLE_ATTACKER].Stats[StatOff + WARSTAT_DEFENSE], CountFrontSize(&Battle->Side[BATTLE_DEFENDER].FrontWarbands[i]));
			RemoveCasualties(&Battle->Side[BATTLE_ATTACKER].FrontWarbands[i], AttkCas, Battle->Side[BATTLE_ATTACKER].FrontSize[i]);
			RemoveCasualties(&Battle->Side[BATTLE_DEFENDER].FrontWarbands[i], DefCas, Battle->Side[BATTLE_DEFENDER].FrontSize[i]);
			if(Battle->Side[BATTLE_ATTACKER].FrontSize[i] - AttkCas < 0)
				Battle->Side[BATTLE_ATTACKER].FrontSize[i] = 0;
			else
				Battle->Side[BATTLE_ATTACKER].FrontSize[i] -= AttkCas;

			if(Battle->Side[BATTLE_DEFENDER].FrontSize[i] - DefCas < 0)
				Battle->Side[BATTLE_DEFENDER].FrontSize[i] = 0;
			else
			Battle->Side[BATTLE_DEFENDER].FrontSize[i] -= DefCas;
		}
	}
}

void* BattleNext(void* Battle) {
	return ((struct Battle*)Battle)->Next;
}

void* BattlePrev(void* Battle) {
	return ((struct Battle*)Battle)->Prev;
}
