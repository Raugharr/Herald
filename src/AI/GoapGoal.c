/*
 * File: GoapGoal.c
 * Author: David Brotz
 */

#include "GoapGoal.h"

#include "goap.h"
#include "GoapAction.h"

#include "../WorldState.h"

#include "../sys/Log.h"

#include <stdlib.h>

void InitGoapGoal(struct GoapGoal* _Goal) {
	_Goal->Name = NULL;
	_Goal->ActionCt = 0;
	_Goal->UtilityFunc = NULL;
	_Goal->Setup = NULL;
	for(int i = 0; i < GOAPGOAL_ATOMS; ++i) {
		for(int j = 0; j < GOAP_ATOMOPS; ++j) {
			_Goal->AtomActions[i][j] = -1;
		}
	}
}

void GoapGSClear(struct GoapGoalSet* _GoalSet) {
	for(int i = 0; i < GOAPGS_GOALMAX; ++i)
		_GoalSet->Goals[i] = NULL;
}

int GoapGoalAddAction(struct GoapGoal* _Goal, const char* _Action) {
	struct GoapAction* _GoapAction = GoapGetAction(_Goal->Planner, _Action);
	struct WorldState _TempState;
	int _AtomIdx = 0;

	if(_GoapAction == NULL)
		return 0;
	_Goal->Actions[_Goal->ActionCt] = _GoapAction;
	++_Goal->ActionCt;
	WorldStateCopy(&_TempState, &_GoapAction->Postconditions);
	_AtomIdx = WorldStateFirstAtom(&_TempState);
	while(_AtomIdx != 0) {
		--_AtomIdx;
		for(int i = 0; i < GOAP_ATOMOPS; ++i) {
			if(_Goal->AtomActions[_AtomIdx][i] == -1) {
				for(int j = 0; j < _Goal->ActionCt; ++j) {
					if(_Goal->Actions[j] == _GoapAction) {
						_Goal->AtomActions[_AtomIdx][i] = j;
						goto loop_end;
					}
				}
			}
				//_Planner->AtomActions[_AtomIdx][i] = (_Action - _Planner->Actions) / sizeof(struct GoapAction);
		}
		loop_end:
		WorldStateClearAtom(&_TempState, _AtomIdx);
		_AtomIdx = WorldStateFirstAtom(&_TempState);
	}
	return 1;
}

const struct GoapAction* GoapGoalBestAction(const struct GoapGoal* _Goal, int _Atom, const struct Agent* _Agent, const struct GoapPathNode* _Node) {
	int _BestCost = 0;
	int _Cost = 0;
	int _BestIdx = 0;
	int _Check = 0;
	const struct GoapAction* _Action = NULL;

	if(_Goal->AtomActions[_Atom][0] != -1);
	_BestIdx = _Goal->AtomActions[_Atom][0];
	_Action = _Goal->Actions[_BestIdx];
	if(_Action == NULL)
		return NULL;
	_BestCost = _Action->Cost(_Agent);

	for(int i = 1; i < _Goal->Planner->AtomCt; ++i) {
		if(_Goal->AtomActions[_Atom][i] == -1)
			return _Action;
		_Action = _Goal->Actions[_Goal->AtomActions[_Atom][i]];
		_Cost = _Action->Cost(_Agent);
		switch(WorldStateGetOpCode(&_Node->State, _Atom)) {
		case WSOP_EQUAL:
			_Check = (_Cost == _BestCost);
			break;
		case WSOP_GREATERTHAN:
			_Check = (_Cost > _BestCost);
			break;
		case WSOP_GREATERTHANEQUAL:
			_Check = (_Cost >= _BestCost);
			break;
		case WSOP_LESSTHAN:
			_Check = (_Cost < _BestCost);
			break;
		case WSOP_LESSTHANEQUAL:
			_Check = (_Cost <= _BestCost);
			break;
		}
		if(_Check != 0) {
			_BestCost = _Cost;
			_BestIdx = _Goal->AtomActions[_Atom][i];
		}
	}
	return _Action;
}
