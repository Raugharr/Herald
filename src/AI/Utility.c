/*
 * File: Utility.c
 * Author: David Brotz
 */

#include "Utility.h"

#include "../sys/Math.h"

#include <string.h>

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
	return _Num;
}

void AUtilityClear(struct AgentUtility* _Planner) {
	_Planner->UtilityCt = 0;
}

void AUtilityAdd(struct AgentUtility* _Planner, const char* _Utility, UtilityCallback _Callback, int _Func) {
	if(_Planner->UtilityCt == UTILITYSZ)
		return;
	_Planner->Utilities[_Planner->UtilityCt] = _Callback;
	_Planner->UtilityFunction[_Planner->UtilityCt] = _Func;
	_Planner->UtilityNames[_Planner->UtilityCt] = _Utility;
	++_Planner->UtilityCt;
}

void AUtilityBest(const struct AgentUtility* _Planner, const void* _Data, struct WorldState* _BestState) {
	struct WorldState _State;
	int _Min = 0;
	int _Max = 0;
	double _Best = 0.0;
	double _Utility = 0.0;

	if(_Planner->UtilityCt < 1)
		return;
	WorldStateClear(&_State);
	_Utility = _Planner->Utilities[0](_Data, &_Min, &_Max, _BestState);
	_Best = AUtilityFunction(Normalize(_Utility, _Min, _Max), _Planner->UtilityFunction[0]);
	for(int i = 1; i < _Planner->UtilityCt; ++i) {
		WorldStateClear(&_State);
		_Utility = _Planner->Utilities[i](_Data, &_Min, &_Max, &_State);
		_Utility = AUtilityFunction(Normalize(_Utility, _Min, _Max), _Planner->UtilityFunction[i]);
		if(_Utility > _Best) {
			_Best =	_Utility;
			WorldStateSetState(_BestState, &_State);
		//	WorldStateCopy(_BestState, &_State);
		}
	}
}

void AUtilityPlan(const struct AgentUtility* _Planner, const void* _Data, const struct WorldState* _State, int* _PathSize, struct GoapPathNode** _Path) {
	struct WorldState _EndState;

	WorldStateClear(&_EndState);
	AUtilityBest(_Planner, _Data, &_EndState);
	WorldStateAdd(&_EndState, _State);
	GoapPlanAction(&_Planner->Planner, _Data, _State, &_EndState, _PathSize, _Path);
}

struct GOAPPlanner* AUtilityGetGoap(struct AgentUtility* _Planner) {
	return &_Planner->Planner;
}
