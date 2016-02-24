/*
 * File: Battle.h
 * Author: David Brotz
 */
#ifndef __BATTLE_H
#define __BATTLE_H

#include "Warband.h"

#include "sys/LinkedList.h"

#define BATTLE_MAXFRONTS (3)

enum {
	BATTLE_RETREAT,
	BATTLE_ADVANCE,
	BATTLE_SKIRMISH,
	BATTLE_ORGANIZE,
	BATTLE_CHARGE,
	BATTLE_ROUT
};

enum {
	BATTLE_MEETING,
	BATTLE_RAID
};

struct FrontSide {
	struct LinkedList WarbandList;
	struct UnitStats Stats;
	int UnitCt;
};

struct Front {
	struct FrontSide Attacker;
	struct FrontSide Defender;
	int IsAlive;
};

struct BattleSide {
	struct Army* Army;
	int StartingSize; //How many soldiers are in the army before the battle starts.
	int Action;
	int Pos;
};

struct Battle {
	struct BattleSide Attacker;
	struct BattleSide Defender;
	int Range; //How far the armies are away from each other.
	struct Front Fronts[BATTLE_MAXFRONTS];
	struct {
		int AttkBegin; //Beginning attackers man count
		int AttkCas;
		int DefBegin;
		int DefCas;
	} Stats;
	struct Battle* Next;
	struct Battle* Prev;
};

int ArmyBattleDecision(const struct Army* _Army,  struct BattleSide* _Side, int _Range);
struct Battle* CreateBattle(struct Army* _Attacker, struct Army* _Defender);
void DestroyBattle(struct Battle* _Battle);

void BattleEnd(int _Victor, struct Battle* _Battle);
void BattleSetupSide(struct Army* _Army, struct FrontSide* _Side);
void BattleGetAction(struct Battle* _Battle, struct BattleSide* _Side);
void BattleThink(struct Battle* _Battle);
void BattleMelee(struct Battle* _Battle);
void BattleIncrRange(struct Battle* _Battle, int _Range);

void BattleSideIncrPos(struct Battle* _Battle, struct BattleSide* _Side, int _Pos);

void* BattleNext(void* _Battle);
void* BattlePrev(void* _Battle);
#endif
