/*
 * File: Action.h
 * Author: David Brotz
 */

#ifndef __ACTION_H
#define __ACTION_H

#include "goap.h"
#include "GoapGoal.h"

#include "actions/ActionRaid.h"
#include "actions/ActionDuel.h"
#include "actions/ActionRemovePolicy.h"
#include "actions/ActionGrowInfluence.h"

#include "goals/GoalUsurpRuler.h"

static AgentActions g_GOAPActionList = {
	ActionRaid,
	ActionDuel,
	ActionRemovePolicy,
	ActionGrowInfluence,
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
