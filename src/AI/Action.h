/*
 * File: Action.h
 * Author: David Brotz
 */

#include "goap.h"

#include "actions/ActionImproveRelations.h"
#include "actions/ActionRaid.h"
#include "actions/ActionDuel.h"

static AgentActions g_GOAPActionList = {
	ActionImproveRelations,
	ActionRaid,
	ActionDuel,
	NULL
};
