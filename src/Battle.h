/*
 * File: Battle.h
 * Author: David Brotz
 */
#ifndef __BATTLE_H
#define __BATTLE_H

#include "Warband.h"

#include "sys/LinkedList.h"

#include <stdint.h>

#define BATTLE_MAXFRONTS (3)
#define BattleFrontStats(Stat, Front) ((Stat) + WARSTAT_SIZE * (Front))

enum {
	PHASE_SKIRMISH,
	PHASE_MELEE,
	PHASE_ROUT
};

enum {
	BATTLE_ATTACKER,
	BATTLE_DEFENDER,
	BATTLE_SIDES
};

enum {
	BATFLANK_LEFT,
	BATFLANK_MID,
	BATFLANK_RIGHT,
	BATFLANK_SIZE
};

struct BattleSide {
	struct Army* Army;
	struct Array FrontWarbands[BATTLE_MAXFRONTS];
	int32_t FrontSize[BATTLE_MAXFRONTS];
	uint8_t Stats[WARSTAT_SIZE * BATTLE_MAXFRONTS];
	uint8_t LeftRout : 1;
	uint8_t RightRout : 1;
	uint8_t MidRout : 1;
	uint16_t StartingSize; //How many soldiers are in the army before the battle starts.
};

struct Battle {
	struct BattleSide Side[BATTLE_SIDES];
	struct {
		uint32_t AttkCas;
		uint32_t DefCas;
	} Stats;
	struct Battle* Next;
	struct Battle* Prev;
	uint8_t Range;
};

int ArmyBattleDecision(const struct Army* Army,  struct BattleSide* Side, int Range);
struct Battle* CreateBattle(struct Army* Attacker, struct Army* Defender);
void DestroyBattle(struct Battle* Battle);

void BattleEnd(int Victor, struct Battle* Battle);
void BattleSetupSide(struct Army* Army, struct BattleSide* Side);
void BattleGetAction(struct Battle* Battle, struct BattleSide* Side);
void BattleThink(struct Battle* Battle);
void BattleMelee(struct Battle* Battle);
void BattleIncrRange(struct Battle* Battle, int Range);

void BattleSideIncrPos(struct Battle* Battle, struct BattleSide* Side, int Pos);

void* BattleNext(void* Battle);
void* BattlePrev(void* Battle);
#endif
