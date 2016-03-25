/*
 * File: goap.c
 * Author: David Brotz
 */

#include "goap.h"

#include "../sys/Math.h"
#include "../sys/MemoryPool.h"
#include "../sys/Log.h"

#include <string.h>

#define GOAP_OPENLIST (512)
#define GOAP_CLOSELIST (128)

static struct MemoryPool* g_GoapMemPool = NULL;

struct GoapPathNode {
	struct WorldState State;
	int Action;
	int ActionCt; //How many times Action is performed.
	const void* Data;
	const struct GoapPathNode* Prev;
	int h;
	int f;
	int g;
};

void GoapInit() {
	g_GoapMemPool = CreateMemoryPool(sizeof(struct GoapPathNode), 65536);
}

void GoapQuit() {
	DestroyMemoryPool(g_GoapMemPool);
}

int GoapNoCost(const void* _Data, const void* _Extra) {
	return 1;
}

int GoapGetActionIndex(struct GOAPPlanner* _Planner, const char* _Action) {
	int i;

	for(i = 0; i < _Planner->ActionCt; ++i) {
		if(strcmp(_Planner->ActionNames[i], _Action) == 0)
			return i;
	}
	if(i < GOAP_ACTIONS) {
		_Planner->ActionNames[i] = _Action;
		_Planner->ActionCosts[i] = GoapNoCost;
		++_Planner->ActionCt;
		return _Planner->ActionCt - 1;
	}
	return -1;
}

int GoapBestAction(const struct GOAPPlanner* _Planner, int _Atom, const void* _Data, const struct GoapPathNode* _Node) {
	int _BestCost = 0;
	int _Cost = 0;
	int _BestIdx = 0;
	int _Check = 0;

	if(_Planner->AtomActions[_Atom][0] != -1);
	_BestIdx = _Planner->AtomActions[_Atom][0];
	_BestCost = _Planner->ActionCosts[_BestIdx](_Data, _Node);

	for(int i = 1; i < _Planner->AtomCt; ++i) {
		if(_Planner->AtomActions[_Atom][i] == -1)
			return _BestIdx;
		_Cost = _Planner->ActionCosts[_Planner->AtomActions[_Atom][i]](_Data, _Node);
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
			_BestIdx = _Planner->AtomActions[_Atom][i];
		}
	}
	return _BestIdx;
}

void GoapClear(struct GOAPPlanner* _Planner) {
	for(int i = 0; i < GOAP_ATOMS; ++i) {
		for(int j = 0; j < GOAP_ATOMOPS; ++j)
			_Planner->AtomActions[i][j] = -1;
		WorldStateClear(&_Planner->Postconditions[i]);
		WorldStateClear(&_Planner->Preconditions[i]);
		_Planner->AtomNames[i] = NULL;
	}
	for(int i = 0; i < GOAP_ACTIONS; ++i) {
		_Planner->ActionCosts[i] = 0;
		_Planner->ActionNames[i] = 0;
	}
	_Planner->ActionCt = 0;
	_Planner->AtomCt = 0;
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

void GoapAddPrecond(struct GOAPPlanner* _Planner, const char* _Action, const char* _Atom, int _Value, int _OpCode) {
	int _ActionIdx = GoapGetActionIndex(_Planner, _Action);

	if(_ActionIdx == -1) {
		Log(ELOG_WARNING, "Action %s does not exist.", _Action);
		return;
	}
	for(int i = 0; i < _Planner->AtomCt; ++i) {
		if(strcmp(_Planner->AtomNames[i], _Atom) == 0) {
			WorldStateSetAtom(&_Planner->Preconditions[_ActionIdx], i, _Value);
			WorldStateSetOpCode(&_Planner->Preconditions[_ActionIdx], i, _OpCode);
			return;
		}
	}
	Log(ELOG_WARNING, "Atom %s was not found.", _Atom);
}

void GoapAddPostcond(struct GOAPPlanner* _Planner, const char* _Action, const char* _Atom, int _Value, int _OpCode) {
	int _ActionIdx = GoapGetActionIndex(_Planner, _Action);
	int _AtomIdx;

	if(_ActionIdx == -1) {
		Log(ELOG_WARNING, "Action %s does not exist.", _Action);
		return;
	}
	for(_AtomIdx = 0; _AtomIdx < _Planner->AtomCt; ++_AtomIdx)
		if(strcmp(_Planner->AtomNames[_AtomIdx], _Atom) == 0)
			goto add_atom_action;
	Log(ELOG_WARNING, "Atom %s was not found.", _Atom);
	return;
	add_atom_action:
	for(int i = 0; i < GOAP_ATOMOPS; ++i) {
		if(_Planner->AtomActions[_AtomIdx][i] == -1) {
			_Planner->AtomActions[_AtomIdx][i] = _ActionIdx;
			goto add_cond;
		}
	}
	return;
	add_cond:
	WorldStateSetAtom(&_Planner->Postconditions[_ActionIdx], _AtomIdx, _Value);
	WorldStateSetOpCode(&_Planner->Postconditions[_ActionIdx], _AtomIdx, _OpCode);
}

void GoapSetActionCost(struct GOAPPlanner* _Planner, const char* _Action, int (*_Cost)(const void*, const void*)) {
	int _ActionIdx = GoapGetActionIndex(_Planner, _Action);

	if(_ActionIdx == -1) {
		Log(ELOG_WARNING, "Atom %s does not exist.", _Action);
		return;
	}
	_Planner->ActionCosts[_ActionIdx] = _Cost;
}

void GoapSetAction(struct GOAPPlanner* _Planner, const char* _ActionName, GOAPAction _Action) {
	int _ActionIdx = GoapGetActionIndex(_Planner, _ActionName);

	if(_ActionIdx == -1) {
		Log(ELOG_WARNING, "Atom %s does not exist.", _ActionName);
		return;
	}
	_Planner->Action[_ActionIdx] = _Action;
}

void CtorGoapPathNode(struct GoapPathNode* _Node, const struct GoapPathNode* _Prev, int _Action, int g, int h) {
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

void GoapPlanAction(const struct GOAPPlanner* _Planner, const void* _Data, const struct WorldState* _Start, struct WorldState* _End, int* _PathSz, struct GoapPathNode** _Path) {
	struct WorldState _CurrentState;
	struct WorldState _ItrState;
	struct GoapPathNode _OpenList[GOAP_OPENLIST];
	int _OpenSize = 0;
	struct GoapPathNode _ClosedList[GOAP_CLOSELIST];
	int _ClosedSize = 0;
	int _BestOpen = 0;
	int _BestOpenIdx = 0;
	int _AtomIdx = 0;
	int _BestAction = 0;

	//Setup _Current state to the state of _Start and to only care about what _End cares about.
	WorldStateClear(&_CurrentState);
	WorldStateSetState(&_CurrentState, _Start);
	WorldStateSetDontCare(&_CurrentState, _End);
	CtorGoapPathNode(&_OpenList[_OpenSize++], NULL, -1, 0, WorldStateDist(_End, &_CurrentState));
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
		_AtomIdx = WorldStateFirstAtom(&_ItrState);
		while(_AtomIdx != 0) {
			/*Choose the best action for (_AtomIdx - 1). If the best action does not satisfy the _End WorldState (_AtomIdx - 1) continue to get the best action
			 * until the best action is different or the _End WorldState (_AtomIdx - 1) is satisfied.
			*/
			do {
				if((_BestAction = GoapBestAction(_Planner, _AtomIdx - 1, _Data, &_ClosedList[_ClosedSize])) == -1)
					break;
				if(_OpenSize == 0) {
					CtorGoapPathNode(&_OpenList[_OpenSize], &_ClosedList[_ClosedSize - 1], _BestAction, _ClosedList[_ClosedSize - 1].g + 1, WorldStateDist(_End, &_CurrentState));
					if(GoapNodeInList(&_OpenList[_OpenSize], _OpenList, _OpenSize, _ClosedList, _ClosedSize) == 0) {
						WorldStateClear(&_OpenList[_OpenSize].State);
						WorldStateCare(&_OpenList[_OpenSize].State);
						WorldStateAdd(&_OpenList[_OpenSize].State, &_Planner->Postconditions[_BestAction]);
						++_OpenSize;
					}
				} else if(_OpenList[_OpenSize - 1].Action == _BestAction) {
					++_OpenList[_OpenSize - 1].ActionCt;
					WorldStateAdd(&_OpenList[_OpenSize - 1].State, &_Planner->Postconditions[_BestAction]);
				}
			} while(WorldStateTruthAtom(&_OpenList[_OpenSize - 1].State, _End, _AtomIdx - 1) == 0);
			WorldStateClearAtom(&_ItrState, _AtomIdx - 1);
			_AtomIdx = WorldStateFirstAtom(&_ItrState);
		}
	}
	if(*_PathSz > _ClosedSize)
		*_PathSz = _ClosedSize;
	for(int i = 0; i < *_PathSz; ++i) {
		_Path[i] = MemPoolAlloc(g_GoapMemPool);
		*_Path[i] = _ClosedList[i];
	}
}

int GoapPathDoAction(const struct GOAPPlanner* _Planner, const struct GoapPathNode* _Node, struct WorldState* _State, void* _Data) {
	int _Cont = 0;

	if(_Node->Action < 0 && _Node->Action < _Planner->ActionCt)
		return 1;
	_Cont = _Planner->Action[_Node->Action](_Data);
	if(_Cont != 0)
		WorldStateAdd(_State, &_Node->State);
	return _Cont;
}

int GoapPathGetAction(const struct GoapPathNode* _Node) {
	return _Node->Action;
}
