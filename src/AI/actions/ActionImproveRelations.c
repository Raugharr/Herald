/*
 * File: ActionImproveRelations.c
 * Author: David Brotz
 */

#include "ActionImproveRelations.h"

#include "../../BigGuy.h"
#include "../../Location.h"
#include "../../Family.h"
#include "../../Person.h"

#include "../../sys/Math.h"

#include "../Agent.h"
#include "../goap.h"

static int ActionCost(const struct Agent* _Agent) {
	return 120;
}

static int ActionFunction(struct Agent* _Agent) {
	struct BigGuy* _Guy = _Agent->Agent;
	struct Settlement* _Settlement = FamilyGetSettlement(_Guy->Person->Family);
	struct LnkLst_Node* _Itr = _Settlement->BigGuys.Front;
	struct BigGuy* _Person = NULL;
	int _Rand = Random(0, _Settlement->BigGuys.Size - 1);
	int _Ct = 0;

	if(_Guy->ActionFunc == NULL) {
		while(_Itr != NULL) {
			_Person = (struct BigGuy*)_Itr->Data;
			if(_Person != _Guy) {
				 if(_Ct >= _Rand)
					 break;
				++_Ct;
			}
			_Itr = _Itr->Next;
		}
		BigGuySetAction(_Guy, BGACT_IMRPOVEREL, _Person, NULL);
		return 1;
	}
	return 0;
}

static int ActionUtility(const struct Agent* _Agent, int* _Min, int* _Max, struct WorldState* _State) {
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
	WorldStateSetAtom(_State, BGBYTE_IMPROVINGRELATION, 1);
	return *_Max;
//	return _Friends;
}

void ActionImproveRelations(struct GOAPPlanner* _GoPlan) {
	GoapAddPostcond(_GoPlan, "Improve Relations", "ImproveRelations", 1, WSOP_ADD);
	GoapSetActionCost(_GoPlan, "Improve Relations", ActionCost);
	GoapSetAction(_GoPlan, "Improve Relations", (AgentActionFunc) ActionFunction);
	GoapAddUtility(_GoPlan, "ImrpoveRelations", (AgentUtilityFunc) ActionUtility, (UTILITY_INVERSE | UTILITY_LINEAR));
}
