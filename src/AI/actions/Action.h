/*
 * File: Action.h
 * Author: David Brotz
 */

#include "../goap.h"

#include "ActionImproveRelations.h"
#include "ActionRaid.h"
#include "ActionDuel.h"

static AgentActions g_GOAPActionList = {
	ActionImproveRelations,
	ActionRaid,
	ActionDuel,
	NULL
};
