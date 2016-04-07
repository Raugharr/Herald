/*
 * File: ActionRaid.c
 * Author: David Brotz
 */

#include "ActionRaid.h"

#include "../../BigGuy.h"
#include "../../Location.h"
#include "../../Family.h"
#include "../../Person.h"
#include "../../Government.h"
#include "../../Warband.h"

#include "../Agent.h"
#include "../goap.h"
#include "../GoapAction.h"

static int ActionCost(const struct Agent* _Agent) {
	return 1;
}

static int ActionFunction(struct Agent* _Agent) {
	struct BigGuy* _Guy = _Agent->Agent;
	struct Settlement* _Settlement = FamilyGetSettlement(_Guy->Person->Family);
	struct ArmyGoal _Goal;
	struct LinkedList _List = {0, NULL, NULL};

	WorldSettlementsInRadius(&g_GameWorld, &_Settlement->Pos, 20, &_List);
	if(_List.Size <= 0)
		goto end;
	ArmyGoalRaid(&_Goal, (struct Settlement*)&_List.Back->Data);
	SettlementRaiseFyrd(_Settlement, &_Goal);
	end:
	LnkLstClear(&_List);
	return 1;
}

static int ActionUtility(const struct Agent* _Agent, int* _Min, int* _Max, struct WorldState* _State) {
	const struct BigGuy* _Guy = _Agent->Agent;
	struct Settlement* _Settlement = FamilyGetSettlement(_Guy->Person->Family);
	int _MaxNutrition = _Settlement->NumPeople * NUTRITION_REQ / 4; //HOw much nutrition we need for 3 months.
	int _Nutrition = SettlementGetNutrition(_Settlement);

	if(_Nutrition >= _MaxNutrition)
		return 0;
	*_Min = 0;
	*_Max = _MaxNutrition;
	WorldStateSetAtom(_State, BGBYTE_FYRDRAISED, 1);
	return _Nutrition;
}

void ActionRaid(struct GOAPPlanner* _Planner, struct GoapAction* _Action) {
	GoapActionAddPrecond(_Action, _Planner, "FyrdRaised", 0, WSOP_EQUAL);
	GoapActionAddPostcond(_Action, _Planner, "FyrdRaised", 1, WSOP_SET);
	GoapActionAddPostcond(_Action, _Planner, "Prestige", 2, WSOP_ADD);
	_Action->Cost = ActionCost;
	_Action->Action = ActionFunction;
	_Action->Utility = ActionUtility;
	_Action->UtilityFunction = (UTILITY_INVERSE | UTILITY_QUADRATIC);
	GoapActionSetName(_Action, "Raid");
}
