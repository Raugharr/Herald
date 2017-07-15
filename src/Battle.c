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
#define BATTLE_SAFERANGE (3)
#define BATTLE_STARTRANGE (BATTLE_SAFERANGE)
#define BATTLE_TICKS (1 * 6)
#define BATTLE_ATTACKER (0)
#define BATTLE_DEFENDER (1)
#define BATTLE_RAND (100)

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

	Battle->Range = 
	Attacker->InBattle = true;
	Defender->InBattle = true;
	Battle->Attacker.Army = Attacker;
	Battle->Attacker.StartingSize = ArmyGetSize(Battle->Attacker.Army);

	Battle->Defender.Army = Defender;
	Battle->Defender.StartingSize = ArmyGetSize(Battle->Defender.Army);
	for(int i = 0; i < BATTLE_MAXFRONTS; ++i) {
		CtorArray(&Battle->Attacker.FrontWarbands[i], Attacker->WarbandCt);
		CtorArray(&Battle->Defender.FrontWarbands[i], Defender->WarbandCt);
		Battle->Attacker.FrontSize[i] = 0;
		Battle->Defender.FrontSize[i] = 0;
	}
	for(struct Warband* Warband = Attacker->Warbands; Warband != NULL; Warband = Warband->Next) {
		ArrayInsert(&Battle->Attacker.FrontWarbands[BATFLANK_MID], Warband);
		Battle->Attacker.FrontSize[BATFLANK_MID] += Warband->Warriors.Size;
	}
	for(int i = 0; i < BATFLANK_SIZE; ++i) {
		Battle->Attacker.FrontStartSize[i] = Battle->Attacker.FrontSize[i];
		Battle->Attacker.FrontRout[i] = false;
	}
	for(struct Warband* Warband = Defender->Warbands; Warband != NULL; Warband = Warband->Next) {
		ArrayInsert(&Battle->Defender.FrontWarbands[BATFLANK_MID], Warband);
		Battle->Defender.FrontSize[BATFLANK_MID] += Warband->Warriors.Size;
	}
	for(int i = 0; i < BATFLANK_SIZE; ++i) {
		Battle->Defender.FrontStartSize[i] = Battle->Defender.FrontSize[i];
		Battle->Defender.FrontRout[i] = false;
	}
	Battle->Range = BATTLE_SAFERANGE;
	for(int i = 0; i < BATTLE_SIDES; ++i) {
		InitUnitStats((uint8_t(*)[WARSTAT_SIZE])&Battle->Attacker.Stats[WARSTAT_SIZE * i]);
		ArmyUpdateStats(Battle->Attacker.Army);
		FrontStats((uint8_t(*)[WARSTAT_SIZE])&Battle->Attacker.Stats[WARSTAT_SIZE * i], &Battle->Attacker.FrontWarbands[i]);

		InitUnitStats((uint8_t(*)[WARSTAT_SIZE])&Battle->Defender.Stats[i]);
		ArmyUpdateStats(Battle->Defender.Army);
		FrontStats((uint8_t(*)[WARSTAT_SIZE])&Battle->Defender.Stats[WARSTAT_SIZE * i], &Battle->Defender.FrontWarbands[i]);
	}
	Battle->BattleSite = NULL;
	Battle->Stats.AttkCas = 0;
	Battle->Stats.DefCas = 0;
	ILL_CREATE(*List, Battle);
	return Battle;
}


void DestroyBattle(struct Battle* Battle) {
	struct Battle** List = (struct Battle**) SubTimeGetList(SUBTIME_BATTLE);

	ILL_DESTROY(*List, Battle);
//	Battle->Attacker.Army->InBattle = false;
//	Battle->Defender.Army->InBattle = false;
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
	struct BigGuy* Guy = NULL;
	int CasteWarriorCt = 0;

	while(Warband != NULL) { 
		for(int i = 0; i < Warband->Warriors.Size; ++i) {
			Warrior = Warband->Warriors.Table[i];
			if(PersonProf(Warrior->Person) != PROF_WARRIOR) {
				continue;
			}
			if((Guy = RBSearch(&g_GameWorld.BigGuys, ((struct Warrior*)Itr->Data)->Person)) != NULL) {
			} else 
			++CasteWarriorCt;
		}
		Warband = Warband->Next;
	}
}

void BattleEnd(int Victor, struct Battle* Battle) {
	//int AttkSize = ArmyGetSize(Battle->Attacker.Army);
	//int DefSize = ArmyGetSize(Battle->Defender.Army);

	//int DefPrestige = (((float)DefSize) / Battle->Defender.StartingSize) / (((float)AttkSize) / Battle->Attacker.StartingSize);
	//int AttkPrestige = (((float)AttkSize) / Battle->Attacker.StartingSize) / (((float)DefSize) / Battle->Defender.StartingSize);

	Battle->Attacker.Army->InBattle = false;
	Battle->Defender.Army->InBattle = false;
	//BattleDistPrestige(&Battle->Attacker, AttkPrestige);
	//BattleDistPrestige(&Battle->Defender, DefPrestige);
	//if(Victor == BATTLE_ATTACKER && Battle->BattleSite != NULL)
		RaidFamilies(&Battle->Attacker.Army->Captives, &Battle->BattleSite->Families, ArmyGetSize(Battle->Attacker.Army));
	PushEvent(EVENT_BATTLE, Battle, NULL);
}


uint16_t BattleDamage(uint8_t HitChance, uint8_t WoundChance, uint8_t BlockChance, uint16_t Rolls) {
	uint16_t Hits = 0;
	uint16_t Cas = 0;

	for(int i = 0; i < Rolls; ++i) {
		if(Random(1, BATTLE_RAND) <= HitChance)
			++Hits;
	}
	for(int i = 0; i < Hits; ++i) {
		uint16_t OffRoll = Random(1, BATTLE_RAND) + WoundChance;
		uint16_t DefRoll = Random(1, BATTLE_RAND) + BlockChance;

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
	int* WarbandDeaths = alloca(sizeof(int) * Front->Size);
	double DeathPercent[Front->Size];
	
	if(ArmySize < 1)
		return;
	for(int i = 0; i < Front->Size; ++i) {
		RandList[i] = Front->Table[i];
		DeathPercent[i] = ((struct Warband*)Front->Table[i])->Warriors.Size / ArmySize;
	}
	CArrayRandom(RandList, Front->Size);
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
		if(Battle->Range > 0) --Battle->Range;
		for(int i = 0; i < BATFLANK_SIZE; ++i) {
			uint8_t StatOff = i * WARSTAT_SIZE;
			uint8_t AttkHit = (Battle->Range == 0) ? (Battle->Attacker.Stats[StatOff + WARSTAT_COMBAT]) : (Battle->Attacker.Stats[StatOff + WARSTAT_RANGEPERCENT]);
			uint8_t AttkBlock = Battle->Attacker.Stats[StatOff + WARSTAT_DEFENSE];
			uint8_t DefHit = (Battle->Range == 0) ? (Battle->Defender.Stats[StatOff + WARSTAT_COMBAT]) : (Battle->Defender.Stats[StatOff + WARSTAT_RANGEPERCENT]);
			uint8_t DefBlock = Battle->Defender.Stats[StatOff + WARSTAT_DEFENSE];

			
			if(Battle->Attacker.FrontRout[i] == true || Battle->Defender.FrontRout[i] == true)
				continue;

			if(Battle->Attacker.FrontRout[i] != true && Battle->Attacker.FrontSize[i] <= Battle->Attacker.FrontStartSize[i] / 2) {
				if(Random(1, BATTLE_RAND) < 50) {
					Battle->Attacker.FrontRout[i] = true;
					AttkBlock /= 2;
				}
			}
			if(Battle->Defender.FrontRout[i] != true && Battle->Defender.FrontSize[i] <= Battle->Defender.FrontStartSize[i] / 2) {
				if(Random(1, BATTLE_RAND) < 50) {
					Battle->Defender.FrontRout[i] = true;
					DefBlock /= 2;
				}
			}
			if(Battle->Attacker.FrontRout[i] == false) {
				DefCas = BattleDamage(AttkHit, AttkHit, DefBlock, CountFrontSize(&Battle->Attacker.FrontWarbands[i]));
			} else {
				DefCas = 0;
			}
			if(Battle->Defender.FrontRout[i] == false)
				AttkCas = BattleDamage(DefHit, DefHit, AttkBlock, CountFrontSize(&Battle->Defender.FrontWarbands[i]));
			else {
				AttkCas = 0;
			}
			if(AttkCas > Battle->Attacker.FrontSize[i]) AttkCas = Battle->Attacker.FrontSize[i];
			if(DefCas > Battle->Defender.FrontSize[i]) AttkCas = Battle->Defender.FrontSize[i];
			RemoveCasualties(&Battle->Attacker.FrontWarbands[i], AttkCas, Battle->Attacker.FrontSize[i]);
			RemoveCasualties(&Battle->Defender.FrontWarbands[i], DefCas, Battle->Defender.FrontSize[i]);
			if(Battle->Attacker.FrontSize[i] - AttkCas < 0)
				Battle->Attacker.FrontSize[i] = 0;
			else
				Battle->Attacker.FrontSize[i] -= AttkCas;

			if(Battle->Defender.FrontSize[i] - DefCas < 0)
				Battle->Defender.FrontSize[i] = 0;
			else
				Battle->Defender.FrontSize[i] -= DefCas;
			Battle->Stats.DefCas += DefCas;
			Battle->Stats.AttkCas += AttkCas;
		}
	}
	BattleEnd((Battle->Stats.AttkCas < Battle->Stats.DefCas) ? (BATTLE_ATTACKER) : (BATTLE_DEFENDER), Battle);
}

void* BattleNext(void* Battle) {
	return ((struct Battle*)Battle)->Next;
}

void* BattlePrev(void* Battle) {
	return ((struct Battle*)Battle)->Prev;
}
