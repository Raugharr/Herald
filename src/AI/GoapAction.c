/*
 * File: GoapAction.c
 * Author: David Brotz
 */

#include "GoapAction.h"

#include "goap.h"

#include <string.h>
#include <malloc.h>

void GoapActionSetName(struct GoapAction* _Action, const char* _Name) {
	_Action->Name = (const char*) calloc(strlen(_Name) + 1, sizeof(char));
	strcpy((char*) _Action->Name, _Name);
}

void GoapActionAddPrecond(struct GoapAction* _Action, struct GOAPPlanner* _Planner, const char* _Atom, int _Value, int _OpCode) {
	for(int i = 0; i < _Planner->AtomCt; ++i) {
		if(strcmp(_Planner->AtomNames[i], _Atom) == 0) {
			WorldStateSetAtom(&_Action->Preconditions, i, _Value);
			WorldStateSetOpCode(&_Action->Preconditions, i, _OpCode);
		}
	}
}

void GoapActionAddPostcond(struct GoapAction* _Action, struct GOAPPlanner* _Planner, const char* _Atom, int _Value, int _OpCode) {
	for(int _AtomIdx = 0;_AtomIdx < _Planner->AtomCt; ++_AtomIdx) {
		if(strcmp(_Planner->AtomNames[_AtomIdx], _Atom) == 0) {
			WorldStateSetAtom(&_Action->Postconditions, _AtomIdx, _Value);
			WorldStateSetOpCode(&_Action->Postconditions, _AtomIdx, _OpCode);
			return;
		}
	}
}
void GoapActionClear(struct GoapAction* _Action) {
	_Action->Name = NULL;
	WorldStateClear(&_Action->Postconditions);
	_Action->ProPostcondition = NULL;
	WorldStateClear(&_Action->Preconditions);
	_Action->ProPrecondition = NULL;
	_Action->Action = NULL;
	_Action->Utility = NULL;
	_Action->Cost = NULL;
	_Action->UtilityFunction = 0;
	_Action->IsComplete = NULL;
}
