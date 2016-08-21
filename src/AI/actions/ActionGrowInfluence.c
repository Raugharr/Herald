/*
 * File: ActionGrowInfluence.c
 * Author: David Brotz
 */

#include "ActionGrowInfluence.h"

#include "../Agent.h"
#include "../goap.h"
#include "../GoapAction.h"

#include "../../sys/LinkedList.h"

#include "../../Retinue.h"
#include "../../Person.h"
#include "../../BigGuy.h"
#include "../../BigGuyRelation.h"
#include "../../Location.h"

enum {
	ACTSTATE_INIT,
	ACTSTATE_CHECK,
	ACTSTATE_END
};

static int ActionCost(const struct Agent* _Agent) {
	return 1;
}

static int ActionFunction(struct Agent* _Agent, void* _Data) {
	struct Retinue* _Retinue = BigGuyRetinue(_Agent->Agent, PersonHome(_Agent->Agent->Person)); 
	struct Settlement* _Home = PersonHome(_Agent->Agent->Person);
	int* _State = _Agent->PlanData;
	
	switch(*_State) {
		case ACTSTATE_INIT:
			if(_Retinue == NULL) {
				_Retinue = CreateRetinue(_Agent->Agent);
				SettlementAddRetinue(_Home, _Retinue);
			}
			_Retinue->IsRecruiting = 1;
			*_State = ACTSTATE_CHECK;
		case ACTSTATE_CHECK:
			if(_Home->FreeWarriors.Size == 0) {
				*_State = ACTSTATE_END;
			}
			break;
		case ACTSTATE_END:
			_Retinue->IsRecruiting = 0;
			break;

	}
	return 1;
}

static int ActionUtility(const struct Agent* _Agent, int* _Min, int* _Max, struct WorldState* _State, void* _Data) {
	const struct Settlement* _Settlement = PersonHome(_Agent->Agent->Person);
	struct Retinue* _Retinue = NULL;
	struct BigGuy* _Actor = NULL;
	int _TotalBG = 0;
	double _PartUtiliy = 0.0f;
	int _Utility = 0;

	for(struct LnkLst_Node* _Itr = _Settlement->BigGuys.Front; _Itr != NULL; _Itr = _Itr->Next) {
		struct BigGuy* _Guy = _Itr->Data;

		if(BigGuyRelAtLeast(BigGuyGetRelation(_Actor, _Guy), BGREL_LIKE) == 0)
			++_TotalBG;
	}
	_Retinue = BigGuyRetinue(_Actor, PersonHome(_Actor->Person));
	_PartUtiliy += _TotalBG / _Settlement->BigGuys.Size * 128;
	_PartUtiliy += _Retinue->Warriors.Size / (_Settlement->MaxWarriors) * 128;
	_Utility = _PartUtiliy;
	*_Min = 0;
	*_Max = 255;
	return _Utility;
}

static int ActionIsComplete(const struct Agent* _Agent, void* _Data) {
	int* _State = _Agent->PlanData;
	return (*_State) == ACTSTATE_END;
}

static void* ActionCreate(const struct Agent* _Agent) {
	int* _State = malloc(sizeof(int));	

	*_State = ACTSTATE_INIT;
	return _State;
}

static void ActionDestroy(void* _Data) {
	free(_Data);
}

void ActionGrowInfluence(struct GOAPPlanner* _Planner, struct GoapAction* _Action) {
	//Both of these should use the ImproveRelations action as a precond.
	GoapActionAddPostcond(_Action, _Planner, "Influence", 255, WSOP_SET);
	GoapActionSetName(_Action, "Grow Influence");
	_Action->Cost = ActionCost;
	_Action->Action = ActionFunction;
	_Action->ProPrecondition = NULL;
	_Action->IsComplete = ActionIsComplete;
	_Action->Utility = ActionUtility;
	_Action->UtilityFunction = UTILITY_LINEAR;
	_Action->Create = ActionCreate;
	_Action->Destroy = ActionDestroy;
}
