/*
 * File: GoapAction.c
 * Author: David Brotz
 */

#include "GoapAction.h"

#include "goap.h"

#include <string.h>

void GoapActionAddPrecond(struct GoapAction* _Action, struct GOAPPlanner* _Planner, const char* _Atom, int _Value, int _OpCode) {
	for(int i = 0; i < _Planner->AtomCt; ++i) {
		if(strcmp(_Planner->AtomNames[i], _Atom) == 0) {
			WorldStateSetAtom(&_Action->Preconditions, i, _Value);
			WorldStateSetOpCode(&_Action->Preconditions, i, _OpCode);
		}
	}
}

void GoapActionAddPostcond(struct GoapAction* _Action, struct GOAPPlanner* _Planner, const char* _Atom, int _Value, int _OpCode) { 
	int _AtomIdx = 0;

	for(;_AtomIdx < _Planner->AtomCt; ++_AtomIdx) {
		if(strcmp(_Planner->AtomNames[_AtomIdx], _Atom) == 0) {
			WorldStateSetAtom(&_Action->Postconditions, _AtomIdx, _Value);
			WorldStateSetOpCode(&_Action->Postconditions, _AtomIdx, _OpCode);
			break;
		}
	}
	for(int i = 0; i < GOAP_ATOMOPS; ++i) {
		if(_Planner->AtomActions[_AtomIdx][i] == -1) {
			for(int j = 0; j < _Planner->ActionCt; ++j) {
				if(&_Planner->Actions[j] == _Action) {
					_Planner->AtomActions[_AtomIdx][i] = j;
					return;
				}
			}
		}
			//_Planner->AtomActions[_AtomIdx][i] = (_Action - _Planner->Actions) / sizeof(struct GoapAction);
	}
}

void GoapActionClear(struct GoapAction* _Action) {
	_Action->Name = NULL;
	WorldStateClear(&_Action->Postconditions);
	WorldStateClear(&_Action->Preconditions);
	_Action->Action = NULL;
	_Action->Utility = NULL;
	_Action->Cost = NULL;
	_Action->UtilityFunction = 0;
	_Action->IsComplete = NULL;
}
