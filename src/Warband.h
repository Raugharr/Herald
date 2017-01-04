/*
 * File: Warband.h
 * Author: David Brotz
 */
#ifndef __WARBAND_H
#define __WARBAND_H

#include "ArmyGoal.h"
#include "Herald.h"

#include "video/Sprite.h"

#include "sys/LinkedList.h"

#include "AI/Pathfind.h"

#include <SDL2/SDL.h>

#include <stdbool.h>

#define WARBAND_MAXMORAL (100)

struct Settlement;
struct BigGuy;
struct Object;
struct Tile;
struct Government;

enum {
	WARSTAT_OFFENSE,
	WARSTAT_DEFENSE,
	WARSTAT_COMBAT,
	WARSTAT_AGILITY,
	WARSTAT_RANGE,
	WARSTAT_RANGEPERCENT,
	WARSTAT_MORAL,
	WARSTAT_SIZE
};

struct ArmyPath {
	struct Path Path;
	struct ArmyPath* Next;
	struct ArmyPath* Prev;
	struct Army* Army;
};

struct UnitStats {
	uint8_t Offense;//Avg str + avg weapon strength.
	uint8_t Defense;//Avg toughness + avg armor.
	uint8_t Combat;//Hit chance.
	uint8_t Agility;
	uint8_t Range;
	uint8_t RangePercent;//Percentage of people who can attack at Range.
	uint8_t Moral;
};

struct Warrior {
	struct Person* Person;
	struct Good* MeleeWeapon;
	struct Good* RangeWeapon;
	struct Good* Armor;
	struct Good* Shield;
};

struct Warband {
	struct Array Warriors;
	struct Settlement* Settlement;
	struct Army* Parent;
	struct BigGuy* Leader;
	uint8_t Stats[WARSTAT_SIZE];
	struct Warband* Next; /* Implicit linked list containing the next and previous warbands in the army that contains this warband.*/
	struct Warband* Prev;
};

struct Army {
	struct Object Object;
	struct Sprite Sprite;
	struct Warband* Warbands; //Implicit linked list.
	struct BigGuy* Leader;
	struct ArmyGoal Goal;
	struct ArmyPath Path; //TODO: might no longer be a needed parameter should be removed.
	uint8_t Stats[WARSTAT_SIZE];
	struct Government* Government;
	struct LinkedList LootedAnimals;
	uint16_t WarbandCt;
	bool InBattle;
	bool CalcPath;
};

void InitUnitStats(uint8_t (*Stats)[WARSTAT_SIZE]);
void UnitStatsClear(struct UnitStats* _Stats);
void UnitStatsAdd(struct UnitStats* _To, const struct UnitStats* _From);
void UnitStatsDiv(struct UnitStats* _Stats, int _Div);
void UnitStatsIncrMoral(struct UnitStats* _Stats, int _Moral);
void CreateWarrior(struct Warband* _Warband, struct Person* _Person, struct Good* _MeleeWeapon, struct Good* _RangeWeapon, struct Good* _Armor, struct Good* _Shield);
void DestroyWarrior(struct Warrior* _Warrior, struct Warband* _Warband);

void CreateWarband(struct Settlement* _Settlement, struct BigGuy* Leader, struct Army* _Army);
void DestroyWarband(struct Warband* _Warband);
void DisbandWarband(struct Warband* _Warband);
int CountWarbandUnits(struct LinkedList* _Warbands);

struct Army* CreateArmy(struct Settlement* _Settlement, struct BigGuy* _Leader, const struct ArmyGoal* _Goal);
void DestroyArmy(struct Army* _Army);
int ArmyPathHeuristic(struct Tile* _One, struct Tile* _Two);

void ArmyThink(struct Army* _Army);
int ArmyGetSize(const struct Army* _Army);
void ArmyCreateFront(struct Army* _Army, struct LinkedList* _Warbands);
void ArmyMove(struct ArmyPath* _ArmyPath);
int ArmyMoveDir(struct Army* _Army, int _Direction);
void ArmyUpdateStats(struct Army* _Army);
void ArmyAddPath(struct Army* _Army, int _EndX, int _EndY);
int ArmyNoPath(const struct Army* _Army);
void ArmyClearPath(struct Army* _Army);

void* ArmyPathNext(void* _Army);
void* ArmyPathPrev(void* _Army);

void ArmyRaidSettlement(struct Army* _Army, struct Settlement* _Settlement);
#endif
