/*
 * File: Battle.c
 * Author: David Brotz
 */

#include "Battle.h"

#include "Herald.h"
#include "Person.h"
#include "BigGuy.h"
#include "Government.h"

#include <stdlib.h>

#define BATTLE_FIRSTFRONT (BATTLE_MAXFRONTS / 2)
#define FRONT_STARTRANGE (0)
#define BATTLE_SAFERANGE (7)
#define BATTLE_TICKS (6)
#define BATTLE_ATTACKER (0)
#define BATTLE_DEFENDER (1)

#define BATTLE_FORMULA(_Stat, _Moral, _Weariness, _Units) ((_Stat) * ((_Moral) * (_Moral)) * ((_Weariness) * (_Weariness)) * (_Units))

int ArmyBattleDecision(const struct Army* _Army, struct BattleSide* _Side, int _Range) {
	int _Status = _Side->Action;

	if(_Side->Army->Stats.Moral < 50)
		return BATTLE_ROUT;
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
	struct Battle* _Battle = NULL;
	struct Battle** _List = (struct Battle**) SubTimeGetList(SUBTIME_BATTLE);
	struct Front* _Front = NULL;
	int i = 0;

	if(_Attacker->WarbandCt <= 0 || _Attacker->InBattle != 0) {
		return NULL;
	}

	if(_Defender->WarbandCt <= 0 || _Attacker->InBattle != 0) {
		return NULL;
	}

	_Battle = (struct Battle*) malloc(sizeof(struct Battle));
	_Attacker->InBattle = 1;
	_Defender->InBattle = 1;
	for(i = 0; i < BATTLE_MAXFRONTS; ++i)
		_Battle->Fronts[i].IsAlive = 0;
	_Front = &_Battle->Fronts[BATTLE_FIRSTFRONT];
	_Front->IsAlive = 1;
	BattleSetupSide(_Attacker, &_Front->Attacker);
	BattleSetupSide(_Defender, &_Front->Defender);
	_Front->Attacker.UnitCt = CountWarbandUnits(&_Front->Attacker.WarbandList);
	_Front->Defender.UnitCt = CountWarbandUnits(&_Front->Defender.WarbandList);
	_Battle->Range = FRONT_STARTRANGE;
	_Battle->Attacker.Army = _Attacker;
	_Battle->Attacker.Action = BATTLE_ORGANIZE;
	_Battle->Attacker.Pos = FRONT_STARTRANGE;
	_Battle->Defender.Army = _Defender;
	_Battle->Defender.Action = BATTLE_ORGANIZE;
	_Battle->Defender.Pos = FRONT_STARTRANGE;
	ILL_CREATE(*_List, _Battle);
	ArmyUpdateStats(_Battle->Attacker.Army);
	ArmyUpdateStats(_Battle->Defender.Army);
	return _Battle;
}

void DestroyBattle(struct Battle* _Battle) {
	struct Battle** _List = (struct Battle**) SubTimeGetList(SUBTIME_BATTLE);

	ILL_DESTROY(*_List, _Battle);
	_Battle->Attacker.Army->InBattle = 0;
	_Battle->Defender.Army->InBattle = 0;
	free(_Battle);
}

void BattleEnd(int _Victor, struct Battle* _Battle) {
	int _AttackSize = ArmyGetSize(_Battle->Attacker.Army);
	int _DefendSize = ArmyGetSize(_Battle->Defender.Army);
	_Battle->Attacker.Army->Leader->Prestige += _AttackSize / _DefendSize;
	_Battle->Defender.Army->Leader->Prestige += _DefendSize / _AttackSize;
	if(_Victor == BATTLE_ATTACKER)
		GovernmentLesserJoin(_Battle->Attacker.Army->Government, _Battle->Defender.Army->Government, GOVREL_TRIBUTE);
	DestroyBattle(_Battle);
}

void BattleSetupSide(struct Army* _Army, struct FrontSide* _Side) {
	const struct Warband* _Warband = NULL;
	const struct LnkLst_Node* _Itr = NULL;
	float _Charge = 0;
	float _Melee = 0;

	_Side->WarbandList.Size = 0;
	_Side->WarbandList.Front = NULL;
	_Side->WarbandList.Back = NULL;
	ArmyCreateFront(_Army, &_Side->WarbandList);
	InitUnitStats(&_Side->Stats);
	_Itr = _Side->WarbandList.Front;
	while(_Itr != NULL) {
		_Warband = (const struct Warband*)_Itr->Data;
		_Charge += _Warband->Stats.Charge;
		_Melee += _Warband->Stats.MeleeAttack;
		_Itr = _Itr->Next;
	}
	_Side->Stats.Charge = _Charge / _Side->WarbandList.Size;
	_Side->Stats.MeleeAttack = _Melee / _Side->WarbandList.Size;;
}

void BattleGetAction(struct Battle* _Battle, struct BattleSide* _Side) {

	_Side->Action = ArmyBattleDecision(_Side->Army, _Side, _Battle->Range);
	switch(_Side->Action) {
	case BATTLE_ADVANCE:
		if(_Battle->Range > 0)
			BattleSideIncrPos(_Battle, _Side, 1);
		break;
	case BATTLE_RETREAT:
		BattleSideIncrPos(_Battle, _Side, -1);
		break;
	case BATTLE_SKIRMISH:
		_Side->Action = BATTLE_ADVANCE;
		break;
	case BATTLE_CHARGE:
		BattleSideIncrPos(_Battle, _Side, 2);
		break;
	case BATTLE_ROUT:
		BattleSideIncrPos(_Battle, _Side, -2);
		break;
	}
}

void BattleThink(struct Battle* _Battle) {
	for(int _Tick = 0; _Tick < BATTLE_TICKS; ++_Tick) {
		if(_Battle->Range >= BATTLE_SAFERANGE) {
			if(_Battle->Attacker.Action == BATTLE_ROUT) {
				BattleEnd(BATTLE_DEFENDER, _Battle);
				return;
			}
			else if(_Battle->Defender.Action == BATTLE_ROUT) {
				BattleEnd(BATTLE_ATTACKER, _Battle);
				return;
			}
		}

		BattleGetAction(_Battle, &_Battle->Attacker);
		BattleGetAction(_Battle, &_Battle->Defender);
		if(_Battle->Range == 0) {
			int _FrontCt = 0;

			BattleMelee(_Battle);
			UnitStatsClear(&_Battle->Attacker.Army->Stats);
			UnitStatsClear(&_Battle->Defender.Army->Stats);
			for(int i = 0; i < BATTLE_MAXFRONTS; ++i) {
				if(_Battle->Fronts[i].IsAlive != 0) {
					UnitStatsAdd(&_Battle->Attacker.Army->Stats, &_Battle->Fronts[i].Attacker.Stats);
					UnitStatsAdd(&_Battle->Defender.Army->Stats, &_Battle->Fronts[i].Defender.Stats);
					++_FrontCt;
				}
			}
			UnitStatsDiv(&_Battle->Attacker.Army->Stats, _FrontCt);
			UnitStatsDiv(&_Battle->Defender.Army->Stats, _FrontCt);
		}
	}
}

void RemoveCasualties(struct LinkedList* _Warbands, float _Amount) {
	struct Warrior* _Warrior = NULL;
	struct Warband* _Warband = NULL;

	while(_Amount >= 1) {
		_Warband = (struct Warband*)_Warbands->Front->Data;
		_Warrior = _Warband->Warriors;
		if(_Warband->Warriors == NULL)
			return;
		DestroyPerson(_Warrior->Person);
		DestroyWarrior(_Warrior, _Warband);
		--_Amount;
	}
}

void BattleMelee(struct Battle* _Battle) {
	float _AttkCas = 0;
	float _DefCas = 0;
	struct Front* _Front = NULL;

	for(int i = 0; i < BATTLE_MAXFRONTS; ++i) {
		_AttkCas = 0;
		_DefCas = 0;
		_Front = &_Battle->Fronts[i];
		if(_Front->IsAlive == 0)
			continue;
		_AttkCas += ((float)BATTLE_FORMULA(_Front->Defender.Stats.MeleeAttack, _Front->Defender.Stats.Moral, _Front->Defender.Stats.Weariness, _Front->Defender.UnitCt)
				/ ((float)BATTLE_FORMULA(_Front->Attacker.Stats.MeleeAttack, _Front->Attacker.Stats.Moral, _Front->Attacker.Stats.Weariness, _Front->Attacker.UnitCt)));
		_DefCas += ((float)BATTLE_FORMULA(_Front->Attacker.Stats.MeleeAttack, _Front->Attacker.Stats.Moral, _Front->Attacker.Stats.Weariness, _Front->Attacker.UnitCt)
				/ ((float)BATTLE_FORMULA(_Front->Defender.Stats.MeleeAttack, _Front->Defender.Stats.Moral, _Front->Defender.Stats.Weariness, _Front->Defender.UnitCt)));
		UnitStatsIncrMoral(&_Front->Attacker.Stats, -((_AttkCas / _Front->Attacker.UnitCt) * 100));
		UnitStatsIncrMoral(&_Front->Defender.Stats, -((_DefCas / _Front->Defender.UnitCt) * 100));
		RemoveCasualties(&_Front->Attacker.WarbandList, _AttkCas);
		RemoveCasualties(&_Front->Defender.WarbandList, _DefCas);
		_Front->Attacker.UnitCt -= _AttkCas;
		_Front->Defender.UnitCt -= _DefCas;
	}
}

void BattleIncrRange(struct Battle* _Battle, int _Range) {
	_Battle->Range += _Range;
	if(_Battle->Range < 0)
		_Battle->Range = 0;
}

void BattleSideIncrPos(struct Battle* _Battle, struct BattleSide* _Side, int _Pos) {
	_Battle->Range -= _Pos;
	if(_Battle->Range < 0) {
		_Battle->Range = 0;
		_Pos += _Battle->Range;
	}

	_Side->Pos += _Pos;
	if(_Side->Pos < 0)
		_Side->Pos = 0;
}

void* BattleNext(void* _Battle) {
	return ((struct Battle*)_Battle)->Next;
}

void* BattlePrev(void* _Battle) {
	return ((struct Battle*)_Battle)->Prev;
}
