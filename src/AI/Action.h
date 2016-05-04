/*
 * File: Action.h
 * Author: David Brotz
 */

#ifndef __ACTION_H
#define __ACTION_H

#include "goap.h"
#include "Goals.h"
#include "GoapGoal.h"

#include "actions/ActionImproveRelations.h"
#include "actions/ActionRaid.h"
#include "actions/ActionDuel.h"
#include "actions/ActionMurder.h"
#include "actions/ActionStealCattle.h"

static AgentActions g_GOAPActionList = {
	ActionImproveRelations,
	ActionRaid,
	ActionDuel,
	ActionMurder,
	ActionStealCattle,
	NULL
};

static AgentGoals g_GOAPGoalList = {
	GoalChallangeLeader,
	NULL
};

const char* g_GoalSetOverthrow[] = {
	"Overthrow Ruler",
	"Challange Leader",
	NULL
};

const char** g_GoapGoalSetList[] = {
	g_GoalSetOverthrow,
	NULL
};
#endif
