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

#define WARBAND_MAXMORAL (100)
#define WARBAND_MAXWEARINESS (100)

struct Settlement;
struct BigGuy;
struct Object;
struct Tile;
struct Government;

struct ArmyPath {
	struct Path Path;
	struct ArmyPath* Next;
	struct ArmyPath* Prev;
	struct Army* Army;
};

struct UnitStats {
	int Range;
	float MeleeAttack;
	float RangeAttack;
	float Charge;
	float Defence;
	int Speed;
	int Moral;
	int Weariness;
};

struct Warrior {
	struct Person* Person;
	struct Good* MeleeWeapon;
	struct Good* RangeWeapon;
	struct Good* Armor;
	struct Good* Shield;
	struct Warrior* Next; /* Implicit linked list containing the next and previous warriors in the warband that contains this warrior.*/
	struct Warrior* Prev;
};

struct Warband {
	struct Warrior* Warriors;
	int WarriorCt;
	struct Settlement* Settlement;
	struct Army* Parent;
	struct UnitStats Stats;
	struct Warband* Next; /* Implicit linked list containing the next and previous warbands in the army that contains this warband.*/
	struct Warband* Prev;
};

struct Army {
	struct Object Object;
	struct Sprite Sprite;
	struct Warband* Warbands; //Implicit linked list.
	int WarbandCt;
	int InBattle;
	struct BigGuy* Leader;
	struct ArmyGoal Goal;
	struct ArmyPath Path; //TODO: might no longer be a needed parameter should be removed.
	struct UnitStats Stats;
	struct Government* Government;
	struct LinkedList LootedAnimals;
};

void InitUnitStats(struct UnitStats* _Stats);
void UnitStatsClear(struct UnitStats* _Stats);
void UnitStatsAdd(struct UnitStats* _To, const struct UnitStats* _From);
void UnitStatsDiv(struct UnitStats* _Stats, int _Div);
void UnitStatsIncrMoral(struct UnitStats* _Stats, int _Moral);
void CreateWarrior(struct Warband* _Warband, struct Person* _Person, struct Good* _MeleeWeapon, struct Good* _RangeWeapon, struct Good* _Armor, struct Good* _Shield);
void DestroyWarrior(struct Warrior* _Warrior, struct Warband* _Warband);

void CreateWarband(struct Settlement* _Settlement, struct Army* _Army);
void DestroyWarband(struct Warband* _Warband);
void DisbandWarband(struct Warband* _Warband);
int CountWarbandUnits(struct LinkedList* _Warbands);

float WarbandGetAttack(struct Warband* _Warband);
float WarbandGetCharge(struct Warband* _Warband);

struct Army* CreateArmy(struct Settlement* _Settlement, const SDL_Point* _Pos, struct Government* _Government, struct BigGuy* _Leader, const struct ArmyGoal* _Goal);
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
