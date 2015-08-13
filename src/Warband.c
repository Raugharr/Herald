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

#include "video/Tile.h"
#include "video/Sprite.h"

#include "sys/LinkedList.h"
#include "sys/Array.h"

#include <stdlib.h>

void InitUnitStats(struct UnitStats* _Stats) {
	_Stats->Moral = WARBAND_MAXMORAL;
	_Stats->Weariness = WARBAND_MAXWEARINESS;
	_Stats->Charge = 0;
	_Stats->Defence = 0;
	_Stats->MeleeAttack = 0;
	_Stats->Range = 0;
	_Stats->RangeAttack = 0;
	_Stats->Speed = 0;
}

void UnitStatsClear(struct UnitStats* _Stats) {
	_Stats->Moral = 0;
	_Stats->Weariness = 0;
	_Stats->Charge = 0;
	_Stats->Defence = 0;
	_Stats->MeleeAttack = 0;
	_Stats->Range = 0;
	_Stats->RangeAttack = 0;
	_Stats->Speed = 0;
}

void UnitStatsAdd(struct UnitStats* _To, const struct UnitStats* _From) {
	_To->Charge += _From->Charge;
	_To->Defence += _From->Defence;
	_To->MeleeAttack += _From->MeleeAttack;
	_To->Moral += _From->Moral;
	_To->Range += _From->Range;
	_To->RangeAttack += _From->RangeAttack;
	_To->Speed += _From->Speed;
	_To->Weariness += _From->Weariness;
}

void UnitStatsDiv(struct UnitStats* _Stats, int _Div) {
	_Stats->Charge /= _Div;
	_Stats->Defence /= _Div;
	_Stats->MeleeAttack /= _Div;
	_Stats->Moral /= _Div;
	_Stats->Range /= _Div;
	_Stats->RangeAttack /= _Div;
	_Stats->Speed /= _Div;
	_Stats->Weariness /= _Div;
}

void UnitStatsIncrMoral(struct UnitStats* _Stats, int _Moral) {
	_Stats->Moral += _Moral;
	if(_Stats->Moral < 0)
		_Stats->Moral = 0;
}

void CreateWarrior(struct Warband* _Warband, struct Person* _Person, struct Good* _MeleeWeapon, struct Good* _RangeWeapon, struct Good* _Armor, struct Good* _Shield) {
	struct Warrior* _Warrior = (struct Warrior*) malloc(sizeof(struct Warrior));

	_Warrior->Person = _Person;
	_Warrior->MeleeWeapon = _MeleeWeapon;
	_Warrior->RangeWeapon = _RangeWeapon;
	_Warrior->Armor = _Armor;
	_Warrior->Shield = _Shield;
	_Warrior->Next = NULL;
	_Warrior->Prev = NULL;
	ILL_CREATE(_Warband->Warriors, _Warrior);
	++_Warband->WarriorCt;
}

void DestroyWarrior(struct Warrior* _Warrior, struct Warband* _Warband) {
	ILL_DESTROY(_Warband->Warriors, _Warrior);
	--_Warband->WarriorCt;
	free(_Warrior);
}

void CreateWarband(struct Settlement* _Settlement, struct Army* _Army) {
	struct Warband* _Warband = NULL;
	struct Person* _Person = _Settlement->People;
	struct Family* _Family = NULL;
	struct Good* _Good = NULL;
	struct Good* _MeleeWeapon = NULL;
	struct Good* _RangeWeapon = NULL;
	struct Good* _Armor = NULL;
	struct Good* _Shield = NULL;

	if(_Settlement == NULL || _Army == NULL || PointInAABB(&_Army->Sprite.TilePos, &_Settlement->Pos) == 0)
		return;

	_Warband = (struct Warband*) malloc(sizeof(struct Warband));
	_Warband->Warriors = NULL;
	_Warband->WarriorCt = 0;
	_Warband->Settlement = _Settlement;
	_Warband->Parent = _Army;
	_Warband->Next = NULL;
	_Warband->Prev = NULL;
	InitUnitStats(&_Warband->Stats);
	while(_Person != NULL) {
		if(PersonIsWarrior(_Person) == 0)
			goto loop_end;
		_Family = _Person->Family;
		for(int i = 0; i < _Family->Goods->Size; ++i) {
			_Good = (struct Good*) _Family->Goods->Table[i];
			if(_Good->Base->Category == EWEAPON) {
				if(((struct WeaponBase*)_Good->Base)->Range == MELEE_RANGE)
					_MeleeWeapon = FamilyTakeGood(_Family, i);
				else
					_RangeWeapon = FamilyTakeGood(_Family, i);
			} else if(_Good->Base->Category == EARMOR) {
				if(((struct ArmorBase*)_Good->Base)->ArmorType == EARMOR_BODY)
					_Armor = FamilyTakeGood(_Family, i);
				else
					_Shield = FamilyTakeGood(_Family, i);
			}
		}
		ILL_DESTROY(_Settlement->People, _Person);
		CreateWarrior(_Warband, _Person, _MeleeWeapon, _RangeWeapon, _Armor, _Shield);
		loop_end:
		_Person = _Person->Next;
	}
	_Warband->Stats.Charge = WarbandGetCharge(_Warband);
	_Warband->Stats.MeleeAttack = WarbandGetAttack(_Warband);
	ILL_CREATE(_Army->Warbands, _Warband);
	++_Army->WarbandCt;
}

void DestroyWarband(struct Warband* _Warband) {
	ILL_DESTROY(_Warband->Parent->Warbands, _Warband);
	--_Warband->Parent->WarbandCt;
	free(_Warband);
}


void DisbandWarband(struct Warband* _Warband) {
	struct Warrior* _Warrior = _Warband->Warriors;

	while(_Warrior != NULL) {
		ILL_CREATE(_Warband->Settlement->People, _Warrior->Person);
		_Warrior = _Warrior->Next;
	}
	DestroyWarband(_Warband);
}

int CountWarbandUnits(struct LinkedList* _Warbands) {
	struct LnkLst_Node* _Itr = _Warbands->Front;
	int _Ct = 0;

	while(_Itr != NULL) {
		_Ct += ((struct Warband*)_Itr->Data)->WarriorCt;
		_Itr = _Itr->Next;
	}
	return _Ct;
}

float WarbandGetAttack(struct Warband* _Warband) {
	struct Warrior* _Warrior = _Warband->Warriors;
	float _TotalAttack = 0.0f;

	while(_Warrior != NULL) {
		if(_Warrior->MeleeWeapon != NULL)
			_TotalAttack += ((struct WeaponBase*)_Warrior->MeleeWeapon->Base)->MeleeAttack;
		_Warrior = _Warrior->Next;
	}
	return _TotalAttack / _Warband->WarriorCt;
}

float WarbandGetCharge(struct Warband* _Warband) {
	struct Warrior* _Warrior = _Warband->Warriors;
	float _TotalCharge = 0.0f;

	while(_Warrior != NULL) {
		if(_Warrior->MeleeWeapon != NULL)
			_TotalCharge += ((struct WeaponBase*)_Warrior->MeleeWeapon->Base)->Charge;
		_Warrior = _Warrior->Next;
	}
	return _TotalCharge / _Warband->WarriorCt;
}

struct Army* CreateArmy(struct Settlement* _Settlement, const SDL_Point* _Pos, struct Government* _Government, struct BigGuy* _Leader, const struct ArmyGoal* _Goal) {
	struct Army* _Army = (struct Army*) malloc(sizeof(struct Army));

	CreateObject((struct Object*) _Army, OBJECT_ARMY, (void(*)(struct Object*))ArmyThink);
	ConstructSprite(&_Army->Sprite, g_GameWorld.MapRenderer, g_GameWorld.MapRenderer->Warrior, MAPRENDER_UNIT, _Pos);
	_Army->Leader = _Leader;
	_Army->WarbandCt = 0;
	_Army->Warbands = NULL;
	_Army->Goal = *_Goal;
	_Army->Path.Path.Direction = TILE_SIZE;
	_Army->Path.Path.Tiles = 0;
	_Army->Path.Path.Next = NULL;
	_Army->Path.Next = NULL;
	_Army->Path.Prev = NULL;
	_Army->Path.Army = NULL;
	_Army->InBattle = 0;
	_Army->Government = _Government;
	CreateWarband(_Settlement, _Army);
	return _Army;
}

void DestroyArmy(struct Army* _Army) {
	DestroyObject((struct Object*) _Army);
	free(_Army->Warbands);
	free(_Army);
}

int ArmyPathHeuristic(struct Tile* _One, struct Tile* _Two) {
	return TileGetDistance(&_One->TilePos, &_Two->TilePos);
}

void ArmyThink(struct Army* _Army) {
	_Army->Goal.Think(_Army);
	if(_Army->WarbandCt == 0) {
		DestroyArmy(_Army);
		return;
	}
	//MapGetUnit(g_GameWorld.MapRenderer, &_Army->Sprite.TilePos);
}

int ArmyGetSize(const struct Army* _Army) {
	struct Warband* _Warband = _Army->Warbands;
	int _Size = 0;

	while(_Warband != NULL) {
		_Size += _Warband->WarriorCt;
		_Warband = _Warband->Next;
	}
	return _Size;
}

void ArmyCreateFront(struct Army* _Army, struct LinkedList* _Warbands) {
	struct Warband* _Warband = _Army->Warbands;

	while(_Warband != NULL) {
		LnkLstPushBack(_Warbands, _Warband);
		_Warband = _Warband->Next;
	}
}

void ArmyMove(struct ArmyPath* _ArmyPath) {
	struct Army* _Army = _ArmyPath->Army;
	struct Path* _Path = &_Army->Path.Path;

	if(_Army->Path.Path.Next == NULL)
		return;
	while(_Path->Direction == TILE_SIZE)
		_Path = _Path->Next;
	do {
		if(_Path->Tiles > 0) {
			if(ArmyMoveDir(_Army, _Path->Direction) != 0)
				--_Path->Tiles;
			return;
		}
		_Path = _Path->Next;
	} while(_Path != NULL);

	_Path = &_Army->Path.Path;
	while(_Path->Next != NULL) {
		_Path = _Path->Next;
		DestroyPath(_Path);
	}
	ArmyClearPath(_Army);
}

int ArmyMoveDir(struct Army* _Army, int _Direction) {
	SDL_Point _Pos;
	struct Settlement* _Settlement = NULL;

	TileAdjTileOffset(&_Army->Sprite.TilePos, _Direction, &_Pos);
	if(_Pos.x == _Army->Sprite.TilePos.x && _Pos.y == _Army->Sprite.TilePos.y)
		return 1;
	if(MapMoveUnit(g_GameWorld.MapRenderer, _Army, &_Pos) != 0) {
		if((_Settlement = MapGetSettlement(g_GameWorld.MapRenderer, &_Army->Sprite.TilePos)) != NULL) {
			if(SettlementIsFriendly(_Settlement, _Army) == 0) {
				if(_Army->Goal.IsRaid != 0) {
					struct Army* _Enemy = NULL;
					struct ArmyGoal _Goal;

					_Enemy = CreateArmy(_Settlement, (struct SDL_Point*)&_Settlement->Pos, _Settlement->Government, _Settlement->Government->Leader, ArmyGoalDefend(&_Goal, _Settlement));
					if(_Enemy != NULL) {
						CreateBattle(_Army, _Enemy);
					}
				} else {
					ArmyRaidSettlement(_Army, _Settlement);
				}
			}
		}
	}
	return 1;
}

void ArmyUpdateStats(struct Army* _Army) {
	struct Warband* _Warband = _Army->Warbands;
	float _Charge = 0;
	float _Melee = 0;
	int _Moral = 0;

	while(_Warband != NULL) {
		_Charge += _Warband->Stats.Charge;
		_Melee += _Warband->Stats.MeleeAttack;
		_Moral += _Warband->Stats.Moral;
		_Warband = _Warband->Next;
	}
	_Army->Stats.Charge = _Charge / _Army->WarbandCt;
	_Army->Stats.MeleeAttack = _Melee / _Army->WarbandCt;
	_Army->Stats.Moral = _Moral / _Army->WarbandCt;
}

void ArmyAddPath(struct Army* _Army, int _EndX, int _EndY) {
	Pathfind(_Army->Sprite.TilePos.x, _Army->Sprite.TilePos.y, _EndX, _EndY, &_Army->Path.Path, _Army, (int(*)(const void*, const void*))&ArmyPathHeuristic, (void(*)(void*, struct Path*))WorldPathCallback);
}

int ArmyNoPath(const struct Army* _Army) {
	return (_Army->Path.Next == NULL && _Army->Path.Path.Direction == TILE_SIZE);
}

void ArmyClearPath(struct Army* _Army) {
	struct ArmyPath** _List = (struct ArmyPath**) SubTimeGetList(SUBTIME_ARMY);

	_Army->Path.Next = NULL;
	_Army->Path.Prev = NULL;
	_Army->Path.Path.Next = NULL;
	_Army->Path.Path.Direction = TILE_SIZE;
	ILL_DESTROY(*_List, &_Army->Path);
}

void* ArmyPathNext(void* _Path) {
	return ((struct ArmyPath*)_Path)->Next;
}

void* ArmyPathPrev(void* _Path) {
	return ((struct ArmyPath*)_Path)->Prev;
}

/*
 * FIXME: Not completed.
 */
void ArmyRaidSettlement(struct Army* _Army, struct Settlement* _Settlement) {
	int _Max = ArmyGetSize(_Army) / 4;
	int _Ct = 0;
	struct SettlementPart* _Part = _Settlement->FirstPart;
	struct LnkLst_Node* _Itr = _Part->Families.Front;
	struct Family* _Family = NULL;

	do {
		do {
			_Family = (struct Family*) _Itr->Data;
			if((++_Ct) < _Max)
				break;
			_Itr = _Itr->Next;
		} while(_Itr != NULL);
		_Part = _Part->Next;
	} while(_Part != NULL);
}
