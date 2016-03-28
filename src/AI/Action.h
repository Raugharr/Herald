/*
 * File: Action.h
 * Author: David Brotz
 */

#ifndef __ACTION_H
#define __ACTION_H

#include "goap.h"
#include "Goals.h"

#include "actions/ActionImproveRelations.h"
#include "actions/ActionRaid.h"
#include "actions/ActionDuel.h"

static AgentActions g_GOAPActionList = {
	ActionImproveRelations,
	ActionRaid,
	ActionDuel,
	NULL
};

static AgentGoals g_GOAPGoalList = {
	GoalChallangeLeader,
	NULL
};
#endif
