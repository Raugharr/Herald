/*
 * File: Warband.h
 * Author: David Brotz
 */
#ifndef __WARBAND_H
#define __WARBAND_H

#include "sys/LinkedList.h"

#define WARBAND_MAXMORAL (100)
#define WARBAND_MAXWEARINESS (100)
#define BATTLE_MAXFRONTS (3)
#define BATTLE_FIRSTFRONT (BATTLE_MAXFRONTS / 2)
#define FRONT_STARTRANGE (0)
#define BATTLE_TICKS (6)

#define BATTLE_FORMULA(_Stat, _Moral, _Weariness, _Units) ((_Stat) * ((_Moral) * (_Moral)) * ((_Weariness) * (_Weariness)) * (_Units))

struct Settlement;
struct BigGuy;

enum {
	BATTLE_RETREAT,
	BATTLE_ADVANCE,
	BATTLE_SKIRMISH,
	BATTLE_ORGANIZE
};

enum {
	BATTLE_MEETING,
	BATTLE_RAID
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
	struct UnitStats Stats;
	struct Warband* Next; /* Implicit linked list containing the next and previous warbands in the army that contains this warband.*/
	struct Warband* Prev;
};

struct Army {
	struct Warband* Warbands;
	int WarbandCt;
	struct BigGuy* Leader;
};

struct Front {
	struct LinkedList Attacker;
	struct UnitStats AttackerStats;
	int AttackerUnits;
	struct LinkedList Defender;
	struct UnitStats DefenderStats;
	int DefenderUnits;
	int IsAlive;
};

struct Battle {
	struct Army* Attacker;
	int AttackerAction;
	struct Army* Defender;
	int DefenderAction;
	int Range;
	struct Front Fronts[BATTLE_MAXFRONTS];
};

void InitUnitStats(struct UnitStats* _Stats);
void CreateWarrior(struct Warband* _Warband, struct Person* _Person, struct Good* _MeleeWeapon, struct Good* _RangeWeapon, struct Good* _Armor, struct Good* _Shield);
void DestroyWarrior(struct Warrior* _Warrior, struct Warband* _Warband);
void CreateWarband(struct Settlement* _Settlement, struct Army* _Army);
void DestroyWarband(struct Warband* _Warband, struct Army* _Army);
int CountWarbandUnits(struct LinkedList* _Warbands);

struct Army* CreateArmy(struct BigGuy* _Leader);
void DestroyArmy(struct Army* _Army);
int ArmyGetSize(const struct Army* _Army);
void ArmyCreateFront(struct Army* _Army, struct LinkedList* _Warbands);
int ArmyBattleDecision(struct Army* _Army, int _Status, int _Range);

struct Battle* CreateBattle(struct Army* _Attacker, struct Army* _Defender);
void BattleThink(struct Battle* _Battle);
void BattleMelee(struct Battle* _Battle);

#endif
