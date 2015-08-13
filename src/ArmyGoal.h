/*
 * File: ArmyGoal.h
 * Author: David Brotz
 */
#ifndef __ARMYGOAL_H
#define __ARMYGOAL_H

struct Army;
struct Settlement;

enum {
	AGOAL_RAID,
	AGOAL_DEFEND,
	AGOAL_REGION,
	AGOAL_SETTLEMENT,
	AGOAL_DISBAND
};

struct ArmyGoal {
	int Type;
	const void* Data;
	void (*Think)(struct Army*);
	int IsRaid;
};

struct ArmyGoal* ArmyGoalRaid(struct ArmyGoal* _Goal, const struct Settlement* _Settlement);
struct ArmyGoal* ArmyGoalDefend(struct ArmyGoal* _Goal, const struct Settlement* _Settlement);

#endif
