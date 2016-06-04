/*
 * Author: David Brotz
 * File: ActionConvince.c
 */

#include "ActionConvince.h"

struct ActionConvinceData {
	struct BigGuy* Target;
	struct BigGuyRelation** RelationTbl;
	int RelationTblSz;
};


static void* ActionCreateData(const struct Agent* _Agent) {
	struct ActionConvinceData* _Data = malloc(sizeof(struct ActionConvinceData));

	_Data->Target = NULL;
	_Data->RelationTbl = calloc(_Agent->Agent.Person.Family.HomeLoc.BigGuys.Size, sizeof(struct BigGuyRelation*));
	_Data->RelationTblSz = 0;
}

static void ActionDestroyData(struct ActionConvinceData* _Data) {
	free(_Data->RelationTbl);
	free(_Data);
}

static int ActionIsComplete(const struct Agent* _Agent, struct ActionConvinceData* _Data) {
	return 1;
}

static int ActionCost(const struct Agent* _Agent) {
	return 1;
}

static int ActionFunction(struct Agent* _Agent, struct ActionConvinceData* _Data) {
	struct BigGuy* _Guy = _Agent->Agent;

	BigGuySetAction(_Guy, BGACT_CONVINCE, _Data->Target, NULL);
	return 1;
}

static int ActionUtility(const struct Agent* _Agent, int* _Min, int* _Max, struct WorldState* _State, struct ActionConvinceData* _Data) {
	const struct BigGuy* _Guy = _Agent->Agent;
	struct BigGuy* _Leader =  _Agent->Blackboard.Target;
	struct BigGuyRelation* _Rel = _Leader->Relations;
	struct Plot* _Plot = GetPlot(_Agent->Agent);
	int _RelTblSz = 0;
	int _Utility = 0;

	*_Min = BIGGUY_RELMIN;
	*_Max = 0;	
	while(_Rel != NULL) {
		if(_Rel->Modifier < 0) {
			_Data->RelationTbl[_Data->RelationTblSz++] = _Rel;
		}
		_Rel = _Rel->Next;
	}
	InsertionSort(_RelTbl, _RelTblSz - 1, NULL, sizeof(struct BigGuyRelation*));
	for(int i = 0; i < _RelTblSz; ++i) {
		if(PlotCanAsk(Plot, _Data->RelationTbl[_Data->RelationTblSz]->Person) != 0) {
			_Target = _Data->RelationTbl[_Data->RelationTblSz]->Person;
			return _Data->RelationTbl[i]->Modifier;	
		}
	}
	return BIGGUY_RELMIN;
}

void ActionConvince(struct GOAPPlanner* _Planner, struct GoapAction* _Action) {
	_Action->Name = "Convince";
	_Action->Cost = ActionCost;
	_Action->Action = ActionFunction;
	_Action->IsComplete = ActionIsComplete;
	_Action->Utility = ActionUtility;
	_Action->UtilityFunction = UTILITY_LINEAR;
}
