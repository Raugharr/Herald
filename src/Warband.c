/*
 * File: Warband.c
 * Author: David Brotz
 */

#include "Warband.h"

#include "Location.h"
#include "Person.h"
#include "Family.h"
#include "Good.h"
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
	struct Warband* _Warband = (struct Warband*) malloc(sizeof(struct Warband));
	struct Person* _Person = _Settlement->People;
	struct Family* _Family = NULL;
	struct Good* _Good = NULL;
	struct Good* _MeleeWeapon = NULL;
	struct Good* _RangeWeapon = NULL;
	struct Good* _Armor = NULL;
	struct Good* _Shield = NULL;
	int i = 0;

	_Warband->Warriors = NULL;
	_Warband->WarriorCt = 0;
	_Warband->Next = NULL;
	_Warband->Prev = NULL;
	while(_Person != NULL) {
		if(_Person->Gender != EMALE || YEAR(_Person->Age) < 15)
			goto loop_end;
		_Family = _Person->Family;
		for(i = 0; i < _Family->Goods->Size; ++i) {
			_Good = (struct Good*) _Family->Goods->Table[i];
			if(_Good->Base->Category == EWEAPON) {
				if(((struct WeaponBase*)_Good->Base)->Range == 1)
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
		CreateWarrior(_Warband, _Person, _MeleeWeapon, _RangeWeapon, _Armor, _Shield);
		loop_end:
		_Person = _Person->Next;
	}
	ILL_CREATE(_Army->Warbands, _Warband);
	++_Army->WarbandCt;
}

void DestroyWarband(struct Warband* _Warband, struct Army* _Army) {
	ILL_DESTROY(_Army->Warbands, _Warband);
	++_Army->WarbandCt;
	free(_Warband);
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

struct Army* CreateArmy(struct BigGuy* _Leader) {
	struct Army* _Army = (struct Army*) malloc(sizeof(struct Army));

	_Army->Leader = _Leader;
	_Army->WarbandCt = 0;
	_Army->Warbands = NULL;
	return _Army;
}

void ArmyCreateFront(struct Army* _Army, struct LinkedList* _Warbands) {
	struct Warband* _Warband = _Army->Warbands;

	while(_Warband != NULL) {
		LnkLstPushBack(_Warbands, _Warband);
		_Warband = _Warband->Next;
	}
}

int ArmyBattleDecision(struct Army* _Army, int _Status, int _Range) {
	switch(_Status) {
	case BATTLE_ORGANIZE:
	case BATTLE_SKIRMISH:
		return _Status - 1;
	case BATTLE_ADVANCE:
		if(_Range == 0)
			return BATTLE_RETREAT;
		return _Status;
	case BATTLE_RETREAT:
		return _Status;
	}
	return -1;
}

struct Battle* CreateBattle(struct Army* _Attacker, struct Army* _Defender) {
	struct Battle* _Battle = (struct Battle*) malloc(sizeof(struct Battle));
	struct Front* _Front = NULL;
	int i = 0;

	for(i = 0; i < BATTLE_MAXFRONTS; ++i)
		_Battle->Fronts[i].IsAlive = 0;
	_Front = &_Battle->Fronts[BATTLE_FIRSTFRONT];
	_Front->IsAlive = 1;
	_Front->Attacker.Size = 0;
	_Front->Attacker.Front = NULL;
	_Front->Attacker.Back = NULL;
	_Front->Defender.Size = 0;
	_Front->Defender.Front = NULL;
	_Front->Defender.Back = NULL;
	InitUnitStats(&_Front->AttackerStats);
	_Front->AttackerStats.Charge = 2;
	_Front->AttackerStats.MeleeAttack = 3;
	InitUnitStats(&_Front->DefenderStats);
	_Front->DefenderStats.Charge = 2;
	_Front->DefenderStats.MeleeAttack = 3;
	_Battle->Range = FRONT_STARTRANGE;
	_Battle->Attacker = _Attacker;
	_Battle->AttackerAction = BATTLE_ORGANIZE;
	_Battle->Defender = _Defender;
	_Battle->DefenderAction = BATTLE_ORGANIZE;
	ArmyCreateFront(_Attacker, &_Front->Attacker);
	ArmyCreateFront(_Defender, &_Front->Defender);
	_Front->AttackerUnits = CountWarbandUnits(&_Front->Attacker);
	_Front->DefenderUnits = CountWarbandUnits(&_Front->Defender);
	return _Battle;
}

void BattleThink(struct Battle* _Battle) {
	int _Result = ArmyBattleDecision(_Battle->Attacker, _Battle->AttackerAction, _Battle->Range);
	if(_Result == BATTLE_ADVANCE)
		--_Battle->Range;
	else if(_Result == BATTLE_RETREAT)
		++_Battle->Range;
	else if(_Result == BATTLE_SKIRMISH)
		_Battle->AttackerAction = BATTLE_ADVANCE;
	_Result = ArmyBattleDecision(_Battle->Defender, _Battle->DefenderAction, _Battle->Range);
	if(_Result == BATTLE_ADVANCE)
		--_Battle->Range;
	else if(_Result == BATTLE_RETREAT)
		++_Battle->Range;
	else if(_Result == BATTLE_SKIRMISH)
			_Battle->DefenderAction = BATTLE_ADVANCE;
	//if(_Battle->Range == 0)
	//	BattleMelee(_Battle);
}

void RemoveCasualties(struct LinkedList* _Warbands, float _Amount) {
	struct Warrior* _Warrior = NULL;
	struct Warband* _Warband = NULL;

	while(_Amount >= 1) {
		_Warband = (struct Warband*)_Warbands->Front->Data;
		_Warrior = _Warband->Warriors;
		DestroyPerson(_Warrior->Person);
		DestroyWarrior(_Warrior, _Warband);
		--_Amount;
	}
}

void BattleMelee(struct Battle* _Battle) {
	int i = 0;
	float _AttkCas = 0;
	float _DefCas = 0;
	struct Front* _Front = NULL;

	for(i = 0; i < BATTLE_MAXFRONTS; ++i) {
		_AttkCas = 0;
		_DefCas = 0;
		_Front = &_Battle->Fronts[i];
		if(_Front->IsAlive == 0)
			continue;
		_AttkCas += ((float)BATTLE_FORMULA(_Front->DefenderStats.MeleeAttack, _Front->DefenderStats.Moral, _Front->DefenderStats.Weariness, _Front->DefenderUnits))
				/ ((float)BATTLE_FORMULA(_Front->AttackerStats.MeleeAttack, _Front->AttackerStats.Moral, _Front->AttackerStats.Weariness, _Front->AttackerUnits));
		_DefCas += ((float)BATTLE_FORMULA(_Front->AttackerStats.MeleeAttack, _Front->AttackerStats.Moral, _Front->AttackerStats.Weariness, _Front->AttackerUnits))
				/ ((float)BATTLE_FORMULA(_Front->DefenderStats.MeleeAttack, _Front->DefenderStats.Moral, _Front->DefenderStats.Weariness, _Front->DefenderUnits));
		RemoveCasualties(&_Front->Attacker, _AttkCas);
		RemoveCasualties(&_Front->Defender, _DefCas);
	}
}
