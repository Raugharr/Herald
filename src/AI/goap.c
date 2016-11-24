/*
 * Author: David Brotz
 * File: goap.c
 */

#include "goap.h"

#include "Blackboard.h"
#include "Agent.h"

#include "../sys/Math.h"
#include "../sys/MemoryPool.h"
#include "../sys/Log.h"

#include <string.h>

#define GOAP_OPENLIST (512)
#define GOAP_CLOSELIST (128)

static struct MemoryPool* g_GoapMemPool = NULL;

void GoapInit() {
	g_GoapMemPool = CreateMemoryPool(sizeof(struct GoapPathNode), 65536);
}

void GoapQuit() {
	if(g_GoapMemPool != NULL)
		DestroyMemoryPool(g_GoapMemPool);
}

int GoapNoCost(const struct Agent* _Agent) {
	return 1;
}

struct GoapAction* GoapGetAction(struct GOAPPlanner* _Planner, const char* _Action) {
	for(int i = 0; i < _Planner->ActionCt; ++i) {
		if(strcmp(_Planner->Actions[i].Name, _Action) == 0)
			return &_Planner->Actions[i];
	}
	Assert("Action is NULL." != NULL);
	return NULL;
}

struct GoapGoal* GoapGetGoal(struct GOAPPlanner* _Planner, const char* _Name) {
	for(int i = 0; i < _Planner->GoalCt; ++i) {
		if(strcmp(_Name, _Planner->Goals[i].Name) == 0)
			return &_Planner->Goals[i];
	}
	return NULL;
}

void GoapClear(struct GOAPPlanner* _Planner) {
	for(int i = 0; i < GOAP_ACTIONS; ++i)
		GoapActionClear(&_Planner->Actions[i]);
	for(int i = 0; i < GOAP_GOALSZ; ++i)
		InitGoapGoal(&_Planner->Goals[i]);
	for(int i = 0; i < GOAP_ATOMS; ++i) 
		_Planner->AtomNames[i] = NULL;
	_Planner->AtomCt = 0;
	_Planner->ActionCt = 0;
	_Planner->GoalCt = 0;
}

void GoapAddAtom(struct GOAPPlanner* _Planner, const char* _Atom) {
	int i;

	for(i = 0; i < _Planner->AtomCt; ++i) {
		if(strcmp(_Planner->AtomNames[i], _Atom) == 0)
			return;
	}
	if(i < GOAP_ACTIONS) {
		_Planner->AtomNames[i] = _Atom;
		++_Planner->AtomCt;
	}
}

void CtorGoapPathNode(struct GoapPathNode* _Node, const struct GoapPathNode* _Prev, const struct GoapAction* _Action, int g, int h) {
	WorldStateClear(&_Node->State);
	WorldStateCare(&_Node->State);
	_Node->Prev = _Prev;
	_Node->Action = _Action;
	_Node->ActionCt = 1;
	_Node->g = g;
	_Node->h = h;
	_Node->f = g + h;
}

int GoapNodeEqual(const struct GoapPathNode* _One, const struct GoapPathNode* _Two) {
	return (WorldStateCmp(&_One->State, &_Two->State) == 0) && (_One->Action == _Two->Action);
}

int GoapNodeInList(const struct GoapPathNode* _Node, struct GoapPathNode* _OpenList, int _OpenSize, struct GoapPathNode* _ClosedList, int _ClosedSize) {
	for(int i = 0; i < _ClosedSize; ++i) {
		if(GoapNodeEqual(_Node, &_ClosedList[i]) == 1) {
			return 1;
		}
	}
	for(int i = 0; i < _OpenSize; ++i) {
		if(GoapNodeEqual(_Node, &_OpenList[i]) == 1) {
			if(_Node->g < _OpenList[i].g) {
				_OpenList[i].g = _Node->g;
			}
			return 1;
		}
	}
	return 0;
}

void GoapPlanAction(const struct GOAPPlanner* _Planner, const struct GoapGoal* _Goal, const struct Agent* _Agent, const struct WorldState* _Start, struct WorldState* _End, uint8_t* _PathSz, struct GoapPathNode** _Path) {
	struct WorldState _CurrentState;
	struct WorldState _ItrState;
	struct GoapPathNode _OpenList[GOAP_OPENLIST];
	int _OpenSize = 0;
	struct GoapPathNode _ClosedList[GOAP_CLOSELIST];
	int _ClosedSize = 0;
	int _BestOpen = 0;
	int _BestOpenIdx = 0;
	int _AtomIdx = 0;

	//Setup _Current state to the state of _Start and to only care about what _End cares about.
	WorldStateClear(&_CurrentState);
	WorldStateSetState(&_CurrentState, _Start);
	WorldStateSetDontCare(&_CurrentState, _End);
	CtorGoapPathNode(&_OpenList[_OpenSize++], NULL, NULL, 0, WorldStateDist(_End, &_CurrentState));
	_OpenList[_OpenSize - 1].State = _CurrentState;
	while(_OpenSize > 0) {
		_BestOpen = _OpenList[0].f;
		_BestOpenIdx = 0;
		for(int i = 1; i < _OpenSize; ++i) {
			if(_OpenList[i].f < _BestOpen) {
				_BestOpen = _OpenList[i].f;
				_BestOpenIdx = i;
				break;
			}
		}
		//Remove best from open list and move to closed list.
		_ClosedList[_ClosedSize++] = _OpenList[_BestOpenIdx];
		WorldStateAdd(&_CurrentState, &_ClosedList[_ClosedSize - 1].State);
		if(WorldStateTruth(&_CurrentState, _End) != 0)
			break;
		if(_OpenSize > 1) {
			_OpenList[_BestOpenIdx] = _OpenList[_OpenSize];
		}
		--_OpenSize;
		WorldStateCopy(&_ItrState, &_CurrentState);
		WorldStateCare(&_ItrState);
		//_AtomIdx = WorldStateFirstAtom(&_ItrState);
		_AtomIdx = 1;
		while(_AtomIdx != 0) {
			/*Choose the best action for (_AtomIdx - 1). If the best action does not satisfy the _End WorldState (_AtomIdx - 1) continue to get the best action
			 * until the best action is different or the _End WorldState (_AtomIdx - 1) is satisfied.
			 */
			for(int i = 0; i < GOAP_ATOMOPS; ++i) {
				if(_Goal->AtomActions[_AtomIdx - 1][i] == -1)
					break;
				const struct GoapAction* const _Action = _Goal->Actions[_Goal->AtomActions[_AtomIdx - 1][i]];

				for(int j = 0; j < _ClosedSize; ++j) {
					if(_ClosedList[j].Action == _Action) {
						goto found;
					}
				}
				for(int j = 0; j < _OpenSize; ++j) {
					if(_OpenList[j].Action == _Action) {
						goto found;
					}
				}
				//Fals cause _Action is equal to 1 and _ItrState is equal to 255 but should pass because of greater than equal op code.`
				for(int _Idx = 0; _Idx < WorldStateBytes; ++_Idx) {
					for(int _Atom = 0; _Atom < WorldStateAtoms; ++_Atom) {
						int _Check = 0;

						if(WSAtomDontCare(&_Action->Preconditions, _Idx, _Atom) == 1)
							continue;
						switch(WSAtomOpCode(&_Action->Preconditions, _Idx, _Atom)) {
							case WSOP_EQUAL:
								_Check = WSToByte(&_Action->Preconditions, _Idx, _Atom) == WSToByte(&_ItrState, _Idx, _Atom);
								break;
							case WSOP_GREATERTHAN:
								_Check = WSToByte(&_Action->Preconditions, _Idx, _Atom) <= WSToByte(&_ItrState, _Idx, _Atom);
								break;
							case WSOP_GREATERTHANEQUAL:
								_Check = WSToByte(&_Action->Preconditions, _Idx, _Atom) < WSToByte(&_ItrState, _Idx, _Atom);
								break;
							case WSOP_LESSTHAN:
								_Check = WSToByte(&_Action->Preconditions, _Idx, _Atom) >= WSToByte(&_ItrState, _Idx, _Atom);
								break;
							case WSOP_LESSTHANEQUAL:
								_Check = WSToByte(&_Action->Preconditions, _Idx, _Atom) < WSToByte(&_ItrState, _Idx, _Atom);
								break;
						}
						if(_Check == 0)
							goto found;
					}
				}
				//Check if the action preconditions are met and if they are not then add the precondition to the dont care list.
				if(_Action->ProPrecondition == NULL || _Action->ProPrecondition(_Agent) != 0) {
					//FIXME: _Temp is here because WorldStateAdd will not add _Action->Postconditions properly
					//because of how it's dontcare bits are set, here we use _Temp to work around that but should
					//be handled because of optimization.
					struct WorldState _Temp;

					WorldStateCopy(&_Temp, &_Action->Postconditions);
					WorldStateCare(&_Temp);
					//FIXME: f cost should include the action's utility.
					CtorGoapPathNode(&_OpenList[_OpenSize], &_ClosedList[_ClosedSize - 1], _Action, _ClosedList[_ClosedSize - 1].g + 1, WorldStateDist(_End, &_CurrentState));
					WorldStateClear(&_OpenList[_OpenSize].State);
					WorldStateCare(&_OpenList[_OpenSize].State);
					WorldStateAdd(&_OpenList[_OpenSize].State, &_Temp);
					++_OpenSize;
				}
			}
			found:
			WorldStateClearAtom(&_ItrState, _AtomIdx - 1);
			//_AtomIdx = WorldStateFirstAtom(&_ItrState);
			if(++_AtomIdx >= WORLDSTATE_ATOMSZ)
				_AtomIdx = 0;
		}
	}
	if(*_PathSz > _ClosedSize)
		*_PathSz = _ClosedSize;
	for(int i = 1; i < *_PathSz; ++i) {
		_Path[i - 1] = MemPoolAlloc(g_GoapMemPool);
		*_Path[i - 1] = _ClosedList[i];
	}
}

int GoapPathDoAction(const struct GOAPPlanner* _Planner, const struct GoapPathNode* _Node, struct WorldState* _State, struct Agent* _Agent) {
	int _Cont = 0;

	if(_Node == NULL || _Node->Action == NULL)
		return 0;
	_Cont = _Node->Action->Action(_Agent, _Node->Data);
	if(_Cont != 0)
		WorldStateAdd(_State, &_Node->State);
	return _Cont;
}

double AUtilityFunction(double _Num, int _Func) {
	switch(_Func) {
		case UTILITY_LINEAR:
			break;
		case UTILITY_QUADRATIC:
			_Num = _Num * _Num;
			break;
	}
	if((_Func & UTILITY_INVERSE) == UTILITY_INVERSE)
		_Num = 1 - _Num;
	return (_Num > 1.0f) ? (1.0f) : (_Num);
}

const struct GoapGoal* GoapBestGoalUtility(const struct GoapGoalSet* const _GoalSet, const struct Agent* _Agent, struct WorldState* _BestState) {
	int _Min = 0;
	int _Max = 0;
	int _BestIdx = 0;
	double _Best = 0.0;
	int _Utility = 0.0;

	if(_GoalSet == NULL)
		return NULL;
	_Utility = _GoalSet->Goals[0]->UtilityFunc(_Agent, &_Min, &_Max);
	_Best = AUtilityFunction(Normalize(_Utility, _Min, _Max), _GoalSet->Goals[0]->Utility);
	for(int i = 1; _GoalSet->Goals[i] != NULL; ++i) {
		_Utility = _GoalSet->Goals[i]->UtilityFunc(_Agent, &_Min, &_Max);
		_Best = AUtilityFunction(Normalize(_Utility, _Min, _Max), _GoalSet->Goals[i]->Utility);
		if(_Utility > _Best) {
			_Best = _Utility;
			_BestIdx = i;
		}
	}
	WorldStateSetState(_BestState, &_GoalSet->Goals[_BestIdx]->GoalState);
	WorldStateSetDontCare(_BestState, &_GoalSet->Goals[_BestIdx]->GoalState);
	//return _GoalSet->Goals[_BestIdx];
	return (_Best >= 0.25f) ? (_GoalSet->Goals[_BestIdx]) : (NULL);
}

void GoapPlanUtility(const struct GOAPPlanner* _Planner, struct Agent* _Agent, struct WorldState* _State,  uint8_t* _PathSize, struct GoapPathNode** _Path) {
	struct WorldState _EndState;
	const struct GoapGoal* _Goal = NULL;
	const struct GoapAction* _Action = NULL ;

	WorldStateClear(&_EndState);
	if((_Goal = GoapBestGoalUtility(_Agent->GoalSet, _Agent, &_EndState)) == NULL)
		return;
	if(_Agent->CurrGoal != _Goal) {
		_Agent->CurrGoal = _Goal;
		_Goal->Setup(_Agent);
	}
	GoapPlanAction(_Planner, _Goal, _Agent, _State, &_EndState, _PathSize, _Path);
	if(*_PathSize == 0)
		return;
	_Action = _Agent->Plan[_Agent->PlanIdx]->Action;
	if(_Agent->PlanData == NULL && _Action->Create != NULL) {
		_Agent->PlanData = _Action->Create(_Agent);
	}
}
