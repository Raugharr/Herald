/*
 * File: ActionImproveRelations.c
 * Author: David Brotz
 */

#include "ActionImproveRelations.h"

#include "../../BigGuy.h"
#include "../../Location.h"
#include "../../Family.h"
#include "../../Person.h"
#include "../../Bulitin.h"

#include "../../sys/Math.h"
#include "../../sys/LuaCore.h"

#include "../Agent.h"
#include "../goap.h"
#include "../GoapAction.h"

static int ActionCost(const struct Agent* _Agent) {
	return 12;
}

static int ActionFunction(struct Agent* _Agent, void* _Data) {
	struct BigGuy* _Guy = _Agent->Agent;
	struct BulitinItem* _Item = _Guy->Person->Family->HomeLoc->Bulitin;

	while(_Item != NULL) {
		if(_Item->Owner == _Agent->Blackboard.Target) {
			BulitinItemOnClick(_Item, g_LuaState, _Guy);
			return 1;
		}
		_Item = _Item->Next;
	}
	return 0;
}

static int IsActionComplete(const struct Agent* _Agent, void* _Data) {
	return 1;
}

static int ActionUtility(const struct Agent* _Agent, int* _Min, int* _Max, struct WorldState* _State, void* _Data) {
	const struct BigGuy* _Guy = _Agent->Agent;
	int _Friends = 0;
	struct Settlement* _Settlement = FamilyGetSettlement(_Guy->Person->Family);
	struct LnkLst_Node* _Itr = _Settlement->BigGuys.Front;
	const struct BigGuy* _Person = NULL;
	const struct BigGuyRelation* _Relation = NULL;

	*_Min = 0;
	*_Max = _Settlement->BigGuys.Size - 1;
	while(_Itr != NULL) {
		_Person = (struct BigGuy*)_Itr->Data;
		if(_Person == _Guy)
			goto end_loop;
		if((_Relation = BigGuyGetRelation(_Guy, _Person)) != NULL && _Relation->Relation >= BGREL_LIKE)
			++_Friends;
		end_loop:
		_Itr = _Itr->Next;
	}
	return *_Max;
}

void ActionImproveRelations(struct GOAPPlanner* _Planner, struct GoapAction* _Action){
	GoapActionAddPostcond(_Action, _Planner, "ImproveRelations", 1, WSOP_ADD);
	_Action->Cost = ActionCost;
	_Action->Action = ActionFunction;
	_Action->Utility = ActionUtility;
	_Action->UtilityFunction = (UTILITY_INVERSE | UTILITY_LINEAR);
	_Action->IsComplete = IsActionComplete;
	_Action->Create = NULL;
	_Action->Destroy = NULL;
	GoapActionSetName(_Action, "Improve Relations");
}
