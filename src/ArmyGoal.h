/*
 * File: ArmyGoal.h
 * Author: David Brotz
 */
#ifndef __ARMYGOAL_H
#define __ARMYGOAL_H

#include <inttypes.h>

struct Army;
struct Settlement;

enum {
	AGOAL_RAID,
	AGOAL_DEFEND,
	AGOAL_REGION,
	AGOAL_SETTLEMENT,
	AGOAL_JOIN,
	AGOAL_DISBAND
};

struct ArmyGoal {
	const void* Data;
	void (*Think)(struct Army*);
	uint8_t Type;
	uint8_t IsRaid;
	uint8_t RaidType;
};

struct ArmyGoal* ArmyGoalRaid(struct ArmyGoal* Goal, const struct Settlement* Settlement, uint8_t RaidType);
struct ArmyGoal* ArmyGoalDefend(struct ArmyGoal* Goal, const struct Settlement* Settlement);
struct ArmyGoal* ArmyGoalJoin(struct ArmyGoal* Goal, struct Army* Army);

#endif
