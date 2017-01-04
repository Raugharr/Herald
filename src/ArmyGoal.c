/*
 * File: ArmyGoal.c
 * Author: David Brotz
 */

#include "ArmyGoal.h"

#include "Warband.h"
#include "Location.h"

#include "video/Sprite.h"
#include "video/QuadTree.h"

void ArmyGoalDisbandThink(struct Army* _Army) {
	struct Settlement* _Settlement = (struct Settlement*) _Army->Goal.Data;

	if(_Army->Sprite.TilePos.x == _Settlement->Pos.x && _Army->Sprite.TilePos.y == _Settlement->Pos.y) {
		struct Warband* _Warband = _Army->Warbands;
		struct Warband* _Temp = NULL;

		while(_Warband != NULL) {
			_Temp = _Warband->Next;
			DisbandWarband(_Warband);
			_Warband = _Temp;
		}
		ArmyClearPath(_Army);
		return;
	}
	if(_Army->Path.Path.Next == NULL && _Army->CalcPath == false) {
//		_Army->CalcPath = true;
		ArmyAddPath(_Army, _Settlement->Pos.x, _Settlement->Pos.y);
	}
}

void ArmyGoalRaidThink(struct Army* _Army) {
	struct Settlement* _Settlement = (struct Settlement*) _Army->Goal.Data;

	if(PointEqual(&_Army->Sprite.TilePos, &_Settlement->Pos) != 0) {
		_Army->Goal.Type = AGOAL_DISBAND;
		_Army->Goal.Data = _Army->Warbands[0].Settlement;
		_Army->Goal.Think = ArmyGoalDisbandThink;
		return;
	}
	if(PointEqual(&_Army->Sprite.TilePos, &_Settlement->Pos) != 0)
		return;
	if(_Army->Path.Path.Next == NULL && _Army->CalcPath == false) {
//		_Army->CalcPath = true;
		ArmyAddPath(_Army, _Settlement->Pos.x, _Settlement->Pos.y);
	}
}

void ArmyGoalDefendThink(struct Army* _Army) {

}

struct ArmyGoal* ArmyGoalRaid(struct ArmyGoal* _Goal, const struct Settlement* _Settlement) {
	_Goal->Type = AGOAL_RAID;
	_Goal->Data = _Settlement;
	_Goal->Think = ArmyGoalRaidThink;
	_Goal->IsRaid = 1;
	return _Goal;
}

struct ArmyGoal* ArmyGoalDefend(struct ArmyGoal* _Goal, const struct Settlement* _Settlement) {
	_Goal->Type = AGOAL_DEFEND;
	_Goal->Data = _Settlement;
	_Goal->Think = ArmyGoalDefendThink;
	_Goal->IsRaid = 0;
	return _Goal;
}
