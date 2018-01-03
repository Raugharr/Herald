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
#include "Profession.h"

#include "video/Tile.h"
#include "video/Sprite.h"

#include "sys/LinkedList.h"
#include "sys/Array.h"
#include "sys/Math.h"
#include "sys/ResourceManager.h"
#include "sys/Log.h"
#include "sys/Rule.h"

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
	struct Family* Family = NULL;
	struct Good* Good = NULL;
	struct Good* MeleeWeapon = NULL;
	struct Good* RangeWeapon = NULL;
	struct Good* Armor = NULL;
	struct Good* Shield = NULL;
	uint8_t MaxRange = 0;

	if(Settlement == NULL || Army == NULL || PointEqual(&Army->Sprite.TilePos, &Settlement->Pos) == 0) {
		Log(ELOG_ERROR, "Cannot create warband, army and settlement are not at same location.");
		return;
	}

	Warband = (struct Warband*) malloc(sizeof(struct Warband));
	CtorArray(&Warband->Warriors, 50);
	Warband->Settlement = Settlement;
	Warband->Parent = Army;
	InitUnitStats(&Warband->Stats);
	for(int i = 0; i < Settlement->People.Size; ++i) {
		struct Person* Person  = Settlement->People.Table[i];

		if(PersonIsWarrior(Person) == 0) continue;
		Family = Person->Family;
		for(int i = 0; i < Family->Goods.Size; ++i) {
			Good = (struct Good*) Family->Goods.Table[i];
			if(Good->Base->Category == GOOD_WEAPON) {
				if(((struct WeaponBase*)Good->Base)->Range == MELEE_RANGE) {
					MeleeWeapon = ArrayRemoveGood(&Family->Goods, i, 1);
				} else {
					struct WeaponBase* Base = NULL;

					RangeWeapon = ArrayRemoveGood(&Family->Goods, i, 1);
					Base = (struct WeaponBase*) RangeWeapon->Base;
					if(Base->Range > MaxRange) MaxRange = Base->Range;
				}
			} else if(Good->Base->Category == GOOD_ARMOR) {
				if(((struct ArmorBase*)Good->Base)->Base.ArmsType == ARMS_BODY) {
					Armor = ArrayRemoveGood(&Family->Goods, i, 1);
				} else {
					Shield = ArrayRemoveGood(&Family->Goods, i, 1);
				}
			}
		}
		SettlementRemovePerson(Settlement, Person);
		Person->Location = Army->Object.Id;
		CreateWarrior(Warband, Person, MeleeWeapon, RangeWeapon, Armor, Shield);
	}
	Warband->Stats[WARSTAT_OFFENSE] = Settlement->Stats[BGSKILL_STRENGTH];
	Warband->Stats[WARSTAT_DEFENSE] = Settlement->Stats[BGSKILL_TOUGHNESS];
	Warband->Stats[WARSTAT_COMBAT] = Settlement->Stats[BGSKILL_COMBAT];
	Warband->Stats[WARSTAT_AGILITY] = Settlement->Stats[BGSKILL_AGILITY];
	Warband->Stats[WARSTAT_RANGE] = MaxRange;
	Warband->Stats[WARSTAT_RANGEPERCENT] = 25;
	Warband->Leader = Leader;
	ArrayInsert_S(&Army->Warbands, Warband);
}

void DestroyWarband(struct Warband* Warband) {
	struct Army* Army = Warband->Parent;
	
	for(int i = 0; i < Army->Warbands.Size; ++i) {
		if(Army->Warbands.Table[i] == Warband) {
			ArrayRemove(&Army->Warbands, i);
			break;
		}
	}
	free(Warband);
}

void DivideSpoils(struct Warband* Warband) {
	float SpoilsRatio = Warband->Warriors.Size / ArmyGetSize(Warband->Parent);
	uint16_t CaptiveCt = Warband->Parent->Captives.Size * SpoilsRatio;
	uint32_t Loot = 0;
	struct Person* Captive = NULL;
	struct Good* Good = NULL;
	struct Person* WarriorList[Warband->Warriors.Size];
	uint32_t WarIdx = 0;

	if(Warband->Warriors.Size == 0)
		return;	
	for(int i = 0; i < Warband->Warriors.Size; ++i)
		WarriorList[i] = Warband->Warriors.Table[i];
	CArrayRandom(WarriorList, Warband->Warriors.Size);
	while(CaptiveCt > 0) {
		Captive = Warband->Parent->Captives.Table[CaptiveCt];
		ArrayRemove(&Warband->Parent->Captives, CaptiveCt);
		if(FamilyAddPerson(WarriorList[WarIdx]->Family, Captive) == false)
			PersonDeath(Captive);

		Good = Warband->Parent->Loot.Table[Loot];
		ArrayAddGood(&WarriorList[WarIdx]->Family->Goods, Good, Loot);
		--CaptiveCt;
		--Loot;
		++WarIdx;
	}
}

void DisbandWarband(struct Warband* Warband) {
	struct Warrior* Warrior = NULL;

	for(int i = 0; i < Warband->Warriors.Size; ++i) {
		Warrior = Warband->Warriors.Table[i];
		SettlementAddPerson(Warband->Settlement, Warrior->Person);
	}
	PushEvent(EVENT_WARBNDHOME, Warband, NULL);
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
	struct Army* Army = CreateObject(OBJECT_ARMY);
	SDL_Point Pos;

	SettlementGetCenter(Settlement, &Pos);
	ConstructGameObject(&Army->Sprite, g_GameWorld.MapRenderer, ResourceGet("Warrior.png"), MAPRENDER_UNIT, &Pos);
	Army->Sprite.Rect.x = 0;
	Army->Sprite.Rect.y = 0;
	
	CtorArray(&Army->Warbands, 8);
	Army->Sprite.Rect.w = 42;
	Army->Sprite.Rect.h = 48;
	Army->Leader = Leader;
	Army->Food = 0;
	Army->Goal = *Goal;
	Army->Path.Path.Direction = TILE_SIZE;
	Army->Path.Path.Tiles = 0;
	Army->Path.Path.Next = NULL;
	Army->Path.Next = NULL;
	Army->Path.Prev = NULL;
	Army->Path.Army = NULL;
	Army->InBattle = 0;
	Army->Government = Settlement->Government;

	CtorArray(&Army->Captives, 0);
	CtorArray(&Army->Loot, 0);
	Army->CalcPath = false;
	CreateWarband(Settlement, Leader, Army);
	ArmyUpdateStats(Army);
	return Army;
}

void DestroyArmy(struct Army* Army) {
	Assert(Army->InBattle == false);

	//for(struct Warband* Warband = Army->Warbands; Warband != NULL; Warband = Warband->Next) {
	for(int i = 0; i < Army->Warbands.Size; ++i) {
		DestroyWarband(Army->Warbands.Table[i]);
	}
	for(int i = 0; i < Army->Captives.Size; ++i) {
		DestroyPerson((struct Person*)Army->Captives.Table[i]);
	}
	for(int i = 0; i < Army->Loot.Size; ++i) {
		DestroyGood((struct Good*)Army->Loot.Table[i]);
	}
	DtorSprite(&Army->Sprite);
	DtorArray(&Army->Warbands);
	DtorArray(&Army->Captives);
	DtorArray(&Army->Loot);
	DestroyObject(&Army->Object);
}

int ArmyPathHeuristic(struct Tile* One, struct Tile* Two) {
	/*
	SDL_Point PosOne;
	SDL_Point PosTwo;

	TileToPos(g_GameWorld.MapRenderer, One, &PosOne);
	TileToPos(g_GameWorld.MapRenderer, Two, &PosTwo);
	return TileGetDistance(&PosOne, &PosTwo);
	*/
	struct CubeCoord CubeOne;
	struct CubeCoord CubeTwo;
	SDL_Point PosOne;
	SDL_Point PosTwo;

	TileToPos(g_GameWorld.MapRenderer, One, &PosOne);
	TileToPos(g_GameWorld.MapRenderer, Two, &PosTwo);
	OffsetToCubeCoord(PosOne.x, PosOne.y, &CubeOne.q, &CubeOne.r, &CubeOne.s);
	OffsetToCubeCoord(PosTwo.x, PosTwo.y, &CubeTwo.q, &CubeTwo.r, &CubeTwo.s);
	return CubeDistance(&CubeOne, &CubeTwo);
}

void ArmyThink(struct Army* Army) {
	if(Army->Warbands.Size == 0) {
		DestroyArmy(Army);
		return;
	}
	Army->Goal.Think(Army);
	//MapGetUnit(g_GameWorld.MapRenderer, &Army->Sprite.TilePos);
}

int ArmyGetSize(const struct Army* Army) {
	int Size = 0;

	for(int i = 0; i < Army->Warbands.Size; ++i) {
		struct Warband* Warband = Army->Warbands.Table[i];

		Size += Warband->Warriors.Size;
	}
	return Size;
}

void ArmyCreateFront(struct Army* Army, struct LinkedList* Warbands) {
	for(int i = 0; i < Army->Warbands.Size; ++i) {
		struct Warband* Warband = Army->Warbands.Table[i];

		LnkLstPushBack(Warbands, Warband);
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

	TileNeighbor(Direction, &Army->Sprite.TilePos, &Pos);
	if(Pos.x == Army->Sprite.TilePos.x && Pos.y == Army->Sprite.TilePos.y)
		return 1;
	if(MapMoveUnit(g_GameWorld.MapRenderer, Army, &Pos) != 0) {
		if((Settlement = WorldGetSettlement(&g_GameWorld, &Army->Sprite.TilePos)) != NULL) {
			if(SettlementIsFriendly(Settlement, Army) == 0) {
				struct Army* Enemy = NULL;
				struct ArmyGoal Goal;

				if(Army->Goal.IsRaid == 0) {
					//ArmyRaidSettlement(Army, Settlement);
					return 1;
				}
				Enemy = CreateArmy(Settlement, Settlement->Government->Leader, ArmyGoalDefend(&Goal, Settlement));
				if(Enemy != NULL) {
					struct Battle* Battle = CreateBattle(Army, Enemy);
					struct Relation* Rel = GetRelation(Army->Government->Relations, Enemy->Government);

					if(Rel == NULL)
						Rel = CreateRelation(Army->Government, Enemy->Government, &Army->Government->Relations);
					ChangeRelation(Rel, ACTTYPE_WAR, -25, OPNLEN_LARGE, OPINION_GREAT);
					Battle->BattleSite = Settlement;
				}
			}
		}
	} else {
		return 0;
	}
	return 1;
}

void ArmyUpdateStats(struct Army* Army) {
	uint32_t Stats[WARSTAT_SIZE] = {0};

	for(int i = 0; i < Army->Warbands.Size; ++i) {
		struct Warband* Warband = Army->Warbands.Table[i];
		
		for(int i = 0; i < WARSTAT_SIZE; ++i) {
			Stats[i] += Warband->Stats[i];
		}
	}

	for(int i = 0; i < WARSTAT_SIZE; ++i) {
		Army->Stats[i] = Stats[i] / Army->Warbands.Size;
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
void RaidFamilies(struct Array* Captives, struct Array* Families, uint32_t MaxCaptives) {
	//uint32_t ArmySize = ArmyGetSize(Army);
	//uint16_t MaxCaptives = ArmySize / 2; 
	int32_t SettlementSz = 61;
	struct Array DeathList;

	CtorArray(&DeathList, 128);
	for(int i = 0; i < Families->Size; ++i) {
		struct Family* Family = Families->Table[i];

			//for(int i = 0; i < Family->NumChildren + CHILDREN; i = (i >= CHILDREN) ? (i) : (i + 1)) {
			for(int i = 0; i < Family->NumChildren + CHILDREN; ++i) {
				struct Person* Person = Family->People[i];

				if(Family->People[i] == NULL || Family->People[i]->Location != Family->HomeLoc->Object.Id)
					continue;
				if(Family->Prof == PROF_WARRIOR || PersonIsWarrior(Person)) {
					ArrayInsert_S(&DeathList, Person);
				} else if(Captives->Size < MaxCaptives) {
					SettlementRemovePerson(Family->HomeLoc, Person);
					//FIXME: Remove from family when they are being transfered to a new city.
					//FamilyRemovePerson(Family, Person);
					ArrayInsert_S(Captives, Person);
					Prisoner(Person, true);
				} else {
					ArrayInsert_S(&DeathList, Person);
				}
			--SettlementSz;
		}
	}
	PersonDeathArr((struct Person**) DeathList.Table, DeathList.Size);
}

void LootFamilies(struct Array* Loot, struct Array* Families, uint32_t MaxGoods) {
	enum {
		ARMYLOOT_GOODSZ = 16,
	};

	//uint32_t ArmySize = ArmyGetSize(Army);
	//uint32_t MaxGoods = ArmySize * 2;
	struct Good* TakenGoods[ARMYLOOT_GOODSZ];
	uint8_t TakenGoodsSz = 0;
	uint16_t TakenQuantity[ARMYLOOT_GOODSZ];
	double Percent = 0;

	for(int i = 0; i < Families->Size; ++i) {
		struct Family* Family = Families->Table[i];
		for(int i = 0; i < Family->Goods.Size; ++i) {
			struct Good* Good = Family->Goods.Table[i];

			
			//Only add goods that can be valuable.
			if(Good->Base->Category == GOOD_FOOD || Good->Base->Category == GOOD_WEAPON || Good->Base->Category == GOOD_ARMOR) {
				for(int i = 0; i < TakenGoodsSz; ++i) {
					if(GoodBaseCmp(TakenGoods[i], Good->Base) == 0) {
						TakenQuantity[i] += Good->Quantity;
						goto no_insert;
					}
				}
				if(TakenGoodsSz < ARMYLOOT_GOODSZ) {
					TakenGoods[TakenGoodsSz++] = Good;
				}
				no_insert:;
			}
		}
	}
	Percent = TakenGoodsSz / MaxGoods;
		Percent = (Percent > 1.0) ? (1.0) : (Percent);
	for(int i = 0; i < TakenGoodsSz; ++i) {
		if(TakenGoods[i]->Base->Category == GOOD_FOOD)
			ArrayAddGood(Loot, TakenGoods[i], TakenGoods[i]->Quantity * Percent * 100);
		else
			ArrayAddGood(Loot, TakenGoods[i], TakenGoods[i]->Quantity * Percent);
	}
}

void WarTypes(struct Array* Families, uint32_t (*RoleCt)[WARROLE_SIZE]) {
	struct Good* Gear[GEAR_SIZE] = {0};
	struct WeaponBase* Weapon = NULL;
	//struct ArmorBase* Armor = NULL;

	for(int Itr = 0; Itr < Families->Size; ++Itr) {
		struct Family* Family = Families->Table[Itr];
		int CurrPerson = 0;

		for(; CurrPerson < Family->NumChildren + CHILDREN; ++CurrPerson) {
			if(PersonIsWarrior(Family->People[CurrPerson]) == false) goto loop_end;
			//goto loop_end;
			//find_gear:
			for(int j = 0; j < Family->Goods.Size; ++j) {
				struct Good* Good = Family->Goods.Table[j];

				if(Good->Base->Category == GOOD_WEAPON) {
					if(((struct WeaponBase*)Good->Base)->Range == MELEE_RANGE && Gear[GEAR_WEPMELEE] == NULL) {
						Gear[GEAR_WEPMELEE] = Good;
					} else if(((struct WeaponBase*)Good->Base)->Range != MELEE_RANGE && Gear[GEAR_WEPRANGE] == NULL){
						Gear[GEAR_WEPRANGE] = Good;
					}
				} else if(Good->Base->Category == GOOD_ARMOR) {
					if(((struct ArmorBase*)Good->Base)->Base.ArmsType== ARMS_BODY && Gear[GEAR_ARMOR] == NULL) {
						Gear[GEAR_ARMOR] = Good;
					} else if(((struct ArmorBase*)Good->Base)->Base.ArmsType == ARMS_SHIELD && Gear[GEAR_SHIELD] == NULL) {
						Gear[GEAR_SHIELD] = Good;
					}
				}
			}
			if(Gear[GEAR_WEPMELEE] != NULL) {
				Weapon = (struct WeaponBase*) Gear[GEAR_WEPMELEE]->Base;
			} else {
				goto support;
			}
			if(Gear[GEAR_SHIELD] != NULL) {
				//Armor = (struct ArmorBase*) Gear[GEAR_SHIELD]->Base;
			} else {
				//goto skirmisher;
			}
			if((Weapon->Range == MELEE_RANGE)) {// && Gear[GEAR_SHIELD] != NULL) {
				if(Gear[GEAR_SHIELD] !=  NULL && Gear[GEAR_ARMOR] != NULL)
					++(*RoleCt)[WARROLE_HEAVYINF];
				else
					++(*RoleCt)[WARROLE_LIGHTINF];
				goto loop_end;
			} 
			//skirmisher:
			if(((struct WeaponBase*)Gear[GEAR_WEPRANGE]->Base)->Base.ArmsType == ARMS_JAVELIN && (Weapon == NULL || Weapon->Base.ArmsType == ARMS_SEAX)) {
				++(*RoleCt)[WARROLE_SKIRMISHER];
				goto loop_end;
			} 
			support:
			if(Gear[GEAR_WEPRANGE] != NULL && ((struct WeaponBase*)Gear[GEAR_WEPRANGE]->Base)->Base.ArmsType == ARMS_BOW) {
				++(*RoleCt)[WARROLE_RANGE];
				goto loop_end;
			}
			loop_end:
			for(int j = 0; j < GEAR_SIZE; ++j) {
				Gear[j] = NULL;
			}
		}
	}
}

