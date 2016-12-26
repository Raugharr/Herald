/*
 * File: Warband.c
 * Author: David Brotz
 */

#include "Warband.h"

#include "Herald.h"
#include "Location.h"
#include "Person.h"
#include "Family.h"
#include "Government.h"
#include "BigGuy.h"
#include "Good.h"
#include "Battle.h"
#include "World.h"
#include "Population.h"

#include "video/Tile.h"
#include "video/Sprite.h"

#include "sys/LinkedList.h"
#include "sys/Array.h"
#include "sys/Math.h"
#include "sys/ResourceManager.h"
#include "sys/Log.h"

#include <stdlib.h>

void InitUnitStats(uint8_t (*Stats)[WARSTAT_SIZE]) {
	for(int i = 0; i < WARSTAT_SIZE; ++i) {
		(*Stats)[i] = 0;
	}
	(*Stats)[WARSTAT_MORAL] = WARBAND_MAXMORAL;
}

/*void UnitStatsClear(struct UnitStats* Stats) {
	Stats->Moral = 0;
	Stats->Weariness = 0;
	Stats->Charge = 0;
	Stats->Defence = 0;
	Stats->MeleeAttack = 0;
	Stats->Range = 0;
	Stats->RangeAttack = 0;
	Stats->Speed = 0;
}

void UnitStatsAdd(struct UnitStats* To, const struct UnitStats* From) {
	To->Charge += From->Charge;
	To->Defence += From->Defence;
	To->MeleeAttack += From->MeleeAttack;
	To->Moral += From->Moral;
	To->Range += From->Range;
	To->RangeAttack += From->RangeAttack;
	To->Speed += From->Speed;
	To->Weariness += From->Weariness;
}

void UnitStatsDiv(struct UnitStats* Stats, int Div) {
	Stats->Charge /= Div;
	Stats->Defence /= Div;
	Stats->MeleeAttack /= Div;
	Stats->Moral /= Div;
	Stats->Range /= Div;
	Stats->RangeAttack /= Div;
	Stats->Speed /= Div;
	Stats->Weariness /= Div;
}*/

void UnitStatsIncrMoral(struct UnitStats* Stats, int Moral) {
	Stats->Moral += Moral;
	if(Stats->Moral < 0)
		Stats->Moral = 0;
}

void CreateWarrior(struct Warband* Warband, struct Person* Person, struct Good* MeleeWeapon, struct Good* RangeWeapon, struct Good* Armor, struct Good* Shield) {
	struct Warrior* Warrior = (struct Warrior*) malloc(sizeof(struct Warrior));

	Warrior->Person = Person;
	Warrior->MeleeWeapon = MeleeWeapon;
	Warrior->RangeWeapon = RangeWeapon;
	Warrior->Armor = Armor;
	Warrior->Shield = Shield;
	ArrayInsert(&Warband->Warriors, Warrior);
}

void DestroyWarrior(struct Warrior* Warrior, struct Warband* Warband) {
	free(Warrior);
}

void CreateWarband(struct Settlement* Settlement, struct BigGuy* Leader, struct Army* Army) {
	struct Warband* Warband = NULL;
	struct Person* Person = Settlement->People;
	struct Family* Family = NULL;
	struct Good* Good = NULL;
	struct Good* MeleeWeapon = NULL;
	struct Good* RangeWeapon = NULL;
	struct Good* Armor = NULL;
	struct Good* Shield = NULL;

	if(Settlement == NULL || Army == NULL || PointEqual(&Army->Sprite.TilePos, &Settlement->Pos) == 0) {
		Log(ELOG_ERROR, "Cannot create warband, army and settlement are not at same location.");
		return;
	}

	Warband = (struct Warband*) malloc(sizeof(struct Warband));
	CtorArray(&Warband->Warriors, 50);
	Warband->Settlement = Settlement;
	Warband->Parent = Army;
	Warband->Next = NULL;
	Warband->Prev = NULL;
	InitUnitStats(&Warband->Stats);
	while(Person != NULL) {
		if(PersonIsWarrior(Person) == 0)
			goto loop_end;
		Family = Person->Family;
		for(int i = 0; i < Family->Goods.Size; ++i) {
			Good = (struct Good*) Family->Goods.Table[i];
			if(Good->Base->Category == GOOD_WEAPON) {
				if(((struct WeaponBase*)Good->Base)->Range == MELEE_RANGE)
					MeleeWeapon = FamilyTakeGood(Family, i, 1);
				else
					RangeWeapon = FamilyTakeGood(Family, i, 1);
			} else if(Good->Base->Category == GOOD_ARMOR) {
				if(((struct ArmorBase*)Good->Base)->ArmorType == EARMOR_BODY)
					Armor = FamilyTakeGood(Family, i, 1);
				else
					Shield = FamilyTakeGood(Family, i, 1);
			}
		}
		ILL_DESTROY(Settlement->People, Person);
		CreateWarrior(Warband, Person, MeleeWeapon, RangeWeapon, Armor, Shield);
		loop_end:
		Person = Person->Next;
	}
	Warband->Stats[WARSTAT_OFFENSE] = Settlement->Stats[BGSKILL_STRENGTH];
	Warband->Stats[WARSTAT_DEFENSE] = Settlement->Stats[BGSKILL_TOUGHNESS];
	Warband->Stats[WARSTAT_COMBAT] = Settlement->Stats[BGSKILL_COMBAT];
	Warband->Stats[WARSTAT_AGILITY] = Settlement->Stats[BGSKILL_AGILITY];
	Warband->Leader = Leader;
	ILL_CREATE(Army->Warbands, Warband);
	++Army->WarbandCt;
}

void DestroyWarband(struct Warband* Warband) {
	ILL_DESTROY(Warband->Parent->Warbands, Warband);
	--Warband->Parent->WarbandCt;
	free(Warband);
}


void DisbandWarband(struct Warband* Warband) {
	struct Warrior* Warrior = NULL;
	float SpoilsRatio = 0;
	uint16_t Spoils = Warband->Parent->LootedAnimals.Size * SpoilsRatio;
	struct Animal* Animal = NULL;
	if(Warband->Warriors.Size == 0)
		goto end;
	SpoilsRatio = Warband->Warriors.Size / ArmyGetSize(Warband->Parent);
	while(Spoils > 0) {
		Animal = Warband->Parent->LootedAnimals.Front->Data;
		LnkLstPopFront(&Warband->Parent->LootedAnimals);
		FamilyAddAnimal(Warband->Leader->Person->Family, Animal);
		--Spoils;
	}
	for(int i = 0; i < Warband->Warriors.Size; ++i) {
		Warrior = Warband->Warriors.Table[i];
		ILL_CREATE(Warband->Settlement->People, Warrior->Person);
	}
	end:
	DestroyWarband(Warband);
}

int CountWarbandUnits(struct LinkedList* Warbands) {
	struct LnkLst_Node* Itr = Warbands->Front;
	int Ct = 0;

	while(Itr != NULL) {
		Ct += ((struct Warband*)Itr->Data)->Warriors.Size;
		Itr = Itr->Next;
	}
	return Ct;
}

/*float WarbandGetAttack(struct Warband* Warband) {
	struct Warrior* Warrior = Warband->Warriors;
	float TotalAttack = 0.0f;

	while(Warrior != NULL) {
		if(Warrior->MeleeWeapon != NULL)
			TotalAttack += ((struct WeaponBase*)Warrior->MeleeWeapon->Base)->MeleeAttack;
		Warrior = Warrior->Next;
	}
	return TotalAttack / Warband->WarriorCt;
}

float WarbandGetCharge(struct Warband* Warband) {
	struct Warrior* Warrior = Warband->Warriors;
	floa TotalCharge = 0.0f;

	while(Warrior != NULL) {
		if(Warrior->MeleeWeapon != NULL)
			TotalCharge += ((struct WeaponBase*)Warrior->MeleeWeapon->Base)->Charge;
		Warrior = Warrior->Next;
	}
	return TotalCharge / Warband->WarriorCt;
}*/

struct Army* CreateArmy(struct Settlement* Settlement, struct BigGuy* Leader, const struct ArmyGoal* Goal) {
	struct Army* Army = (struct Army*) malloc(sizeof(struct Army));
	SDL_Point Pos;

	SettlementGetCenter(Settlement, &Pos);
	CreateObject((struct Object*) Army, OBJECT_ARMY, (void(*)(struct Object*))ArmyThink);
	ConstructGameObject(&Army->Sprite, g_GameWorld.MapRenderer, ResourceGet("Warrior.png"), MAPRENDER_UNIT, &Pos);
	Army->Sprite.Rect.x = 0;
	Army->Sprite.Rect.y = 0;

	Army->Sprite.Rect.w = 42;
	Army->Sprite.Rect.h = 48;
	Army->Leader = Leader;
	Army->WarbandCt = 0;
	Army->Warbands = NULL;
	Army->Goal = *Goal;
	Army->Path.Path.Direction = TILE_SIZE;
	Army->Path.Path.Tiles = 0;
	Army->Path.Path.Next = NULL;
	Army->Path.Next = NULL;
	Army->Path.Prev = NULL;
	Army->Path.Army = NULL;
	Army->InBattle = 0;
	Army->Government = Settlement->Government;

	Army->LootedAnimals.Size = 0;
	Army->LootedAnimals.Front = NULL;
	Army->LootedAnimals.Back = NULL;
	CreateWarband(Settlement, Leader, Army);
	ArmyUpdateStats(Army);
	return Army;
}

void DestroyArmy(struct Army* Army) {
	for(struct Warband* Warband = Army->Warbands; Warband != NULL; Warband = Warband->Next) {
		DestroyWarband(Army->Warbands);
	}
	for(struct LnkLst_Node* Itr = Army->LootedAnimals.Front; Itr != NULL; Itr = Itr->Next) {
		DestroyAnimal((struct Animal*)Itr->Data);
	}
	DestroyObject((struct Object*) Army);
	LnkLstClear(&Army->LootedAnimals);
	free(Army->Warbands);
	free(Army);
}

int ArmyPathHeuristic(struct Tile* One, struct Tile* Two) {
	SDL_Point PosOne;
	SDL_Point PosTwo;

	TileToPos(g_GameWorld.MapRenderer, One, &PosOne);
	TileToPos(g_GameWorld.MapRenderer, Two, &PosTwo);
	return TileGetDistance(&PosOne, &PosTwo);
}

void ArmyThink(struct Army* Army) {
	Army->Goal.Think(Army);
	if(Army->WarbandCt == 0) {
		DestroyArmy(Army);
		return;
	}
	//MapGetUnit(g_GameWorld.MapRenderer, &Army->Sprite.TilePos);
}

int ArmyGetSize(const struct Army* Army) {
	struct Warband* Warband = Army->Warbands;
	int Size = 0;

	while(Warband != NULL) {
		Size += Warband->Warriors.Size;
		Warband = Warband->Next;
	}
	return Size;
}

void ArmyCreateFront(struct Army* Army, struct LinkedList* Warbands) {
	struct Warband* Warband = Army->Warbands;

	while(Warband != NULL) {
		LnkLstPushBack(Warbands, Warband);
		Warband = Warband->Next;
	}
}

void ArmyMove(struct ArmyPath* ArmyPath) {
	struct Army* Army = ArmyPath->Army;
	struct Path* Path = &Army->Path.Path;

	if(Army->Path.Path.Next == NULL)
		return;
	while(Path->Direction == TILE_SIZE)
		Path = Path->Next;
	do {
		if(Path->Tiles > 0) {
			if(ArmyMoveDir(Army, Path->Direction) != 0)
				--Path->Tiles;
			return;
		}
		Path = Path->Next;
	} while(Path != NULL);

	Path = &Army->Path.Path;
	while(Path->Next != NULL) {
		Path = Path->Next;
		DestroyPath(Path);
	}
	ArmyClearPath(Army);
}

int ArmyMoveDir(struct Army* Army, int Direction) {
	SDL_Point Pos;
	struct Settlement* Settlement = NULL;

	TileAdjTileOffset(&Army->Sprite.TilePos, Direction, &Pos);
	if(Pos.x == Army->Sprite.TilePos.x && Pos.y == Army->Sprite.TilePos.y)
		return 1;
	if(MapMoveUnit(g_GameWorld.MapRenderer, Army, &Pos) != 0) {
		if((Settlement = WorldGetSettlement(&g_GameWorld, &Army->Sprite.TilePos)) != NULL) {
			if(SettlementIsFriendly(Settlement, Army) == 0) {
				if(Army->Goal.IsRaid != 0) {
					struct Army* Enemy = NULL;
					struct ArmyGoal Goal;

					Enemy = CreateArmy(Settlement, Settlement->Government->Leader, ArmyGoalDefend(&Goal, Settlement));
					if(Enemy != NULL) {
						CreateBattle(Army, Enemy);
					}
				} else {
					ArmyRaidSettlement(Army, Settlement);
				}
			}
		}
	}
	return 1;
}

void ArmyUpdateStats(struct Army* Army) {
	struct Warband* Warband = Army->Warbands;
	uint32_t Stats[WARSTAT_SIZE] = {0};

	while(Warband != NULL) {
		for(int i = 0; i < WARSTAT_SIZE; ++i) {
			Stats[i] += Warband->Stats[i];
		}
		Warband = Warband->Next;
	}
	for(int i = 0; i < WARSTAT_SIZE; ++i) {
		Army->Stats[i] = Stats[i] / Army->WarbandCt;
	}
}

void ArmyAddPath(struct Army* Army, int EndX, int EndY) {
	Pathfind(Army->Sprite.TilePos.x, Army->Sprite.TilePos.y, EndX, EndY, &Army->Path.Path, Army, (int(*)(const void*, const void*))&ArmyPathHeuristic, (void(*)(void*, struct Path*))WorldPathCallback);
}

int ArmyNoPath(const struct Army* Army) {
	return (Army->Path.Next == NULL && Army->Path.Path.Direction == TILE_SIZE);
}

void ArmyClearPath(struct Army* Army) {
	struct ArmyPath** List = (struct ArmyPath**) SubTimeGetList(SUBTIME_ARMY);

	Army->Path.Next = NULL;
	Army->Path.Prev = NULL;
	Army->Path.Path.Next = NULL;
	Army->Path.Path.Direction = TILE_SIZE;
	ILL_DESTROY(*List, &Army->Path);
}

void* ArmyPathNext(void* Path) {
	return ((struct ArmyPath*)Path)->Next;
}

void* ArmyPathPrev(void* Path) {
	return ((struct ArmyPath*)Path)->Prev;
}

/*
 * FIXME: Take a random amount of goods from a random amount of families.
 */
void ArmyRaidSettlement(struct Army* Army, struct Settlement* Settlement) {
	int AnimalsTaken = ArmyGetSize(Army) / 2 * (Army->Leader->Stats[BGSKILL_AGILITY] / 100.0f);
	struct Family* Family = NULL;

	Assert(Army->Sprite.TilePos.x == Settlement->Pos.y && Army->Sprite.TilePos.y == Settlement->Pos.y);
	while(AnimalsTaken > 0) {
		Family = (struct Family*) LnkLstRandom(&Settlement->Families);
		LnkLstPushBack(&Army->LootedAnimals, FamilyTakeAnimal(Family, Random(0, Family->Animals.Size - 1)));
		--AnimalsTaken;
	}
}
