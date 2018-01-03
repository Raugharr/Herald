/*
 * File: ArmyGoal.c
 * Author: David Brotz
 */

#include "ArmyGoal.h"

#include "Warband.h"
#include "Location.h"

#include "video/Sprite.h"
#include "video/QuadTree.h"

void ArmyGoalDisbandThink(struct Army* Army) {
	struct Settlement* Settlement = (struct Settlement*) Army->Goal.Data;

	if(Army->Sprite.TilePos.x == Settlement->Pos.x && Army->Sprite.TilePos.y == Settlement->Pos.y) {
		for(int i = 0; i < Army->Warbands.Size; ++i) {
			struct Warband* Warband = Army->Warbands.Table[i];

			DisbandWarband(Warband);
		}
		ArmyClearPath(Army);
		return;
	}
	if(Army->Path.Path.Next == NULL && Army->CalcPath == false) {
//		Army->CalcPath = true;
		ArmyAddPath(Army, Settlement->Pos.x, Settlement->Pos.y);
	}
}

void ArmyGoalRaidThink(struct Army* Army) {
	struct Settlement* Settlement = (struct Settlement*) Army->Goal.Data;

	if(PointEqual(&Army->Sprite.TilePos, &Settlement->Pos) != 0) {
		Army->Goal.Type = AGOAL_DISBAND;
		Army->Goal.Data = ((struct Warband*)Army->Warbands.Table[0])->Settlement;
		Army->Goal.Think = ArmyGoalDisbandThink;
		return;
	}
	if(PointEqual(&Army->Sprite.TilePos, &Settlement->Pos) != 0)
		return;
	if(Army->Path.Path.Next == NULL && Army->CalcPath == false) {
//		Army->CalcPath = true;
		ArmyAddPath(Army, Settlement->Pos.x, Settlement->Pos.y);
	}
}

void ArmyGoalJoinThink(struct Army* Army) {

}

void ArmyGoalDefendThink(struct Army* Army) {
	DestroyArmy(Army);
}

struct ArmyGoal* ArmyGoalRaid(struct ArmyGoal* Goal, const struct Settlement* Settlement, uint8_t RaidType) {
	Goal->Type = AGOAL_RAID;
	Goal->Data = Settlement;
	Goal->Think = ArmyGoalRaidThink;
	Goal->IsRaid = 1;
	Goal->RaidType = RaidType;
	return Goal;
}

struct ArmyGoal* ArmyGoalDefend(struct ArmyGoal* Goal, const struct Settlement* Settlement) {
	Goal->Type = AGOAL_DEFEND;
	Goal->Data = Settlement;
	Goal->Think = ArmyGoalDefendThink;
	Goal->IsRaid = 0;
	Goal->RaidType = ARMYGOAL_NONE;
	return Goal;
}

struct ArmyGoal* ArmyGoalJoin(struct ArmyGoal* Goal, struct Army* Army) {
	Goal->Type = AGOAL_JOIN;
	Goal->Data = Army;
	Goal->Think = ArmyGoalJoinThink;
	Goal->IsRaid = 0;
	Goal->RaidType = ARMYGOAL_NONE;
	return Goal;
}
