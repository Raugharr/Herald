/*
 * Author: David Brotz
 * File: Constraint.c
 */

#include "Constraint.h"

#include <stdlib.h>
#include <stdarg.h>

struct Constraint* CreateConstraint(int _Min, int _Max) {
	struct Constraint* _Constraint = (struct Constraint*) malloc(sizeof(struct Constraint));

	_Constraint->Min = _Min;
	_Constraint->Max = _Max;
	return _Constraint;
}

void DestroyConstraint(struct Constraint* _Constraint) {
	free(_Constraint);
}

void DestroyConstrntBnds(struct Constraint** _Constraint) {
	while((*_Constraint) != NULL) {
		free(*_Constraint);
		++_Constraint;
	}
	free(_Constraint);

}

struct Constraint** CreateConstrntLst(int* _Size, int _Min, int _Max, int _Interval) {
	int _CurrMin = _Min;
	int _CurrMax = _Max;
	struct Constraint** _List = NULL;
	int i;

	*_Size = _Max / _Interval;
	if(_Interval * *_Size < _Max)
		++(*_Size);
	_List = (struct Constraint**) malloc(sizeof(struct Constraint) * *_Size);
	for(i = 0; i < *_Size; ++i) {
		if(_CurrMax > _Max) {
			_List[i] = CreateConstraint(_CurrMin, _Max);
			_List[i + 1] = NULL;
			return _List;
		}
		_List[i] = CreateConstraint(_CurrMin, _CurrMax);
		_CurrMin += _Interval;
		_CurrMax += _Interval;
	}
	_List[i + 1] = NULL;
	return _List;
}

struct Constraint** CreateConstrntBnds(int _Size, ...) {
	int _CurrMin = -1;
	int _CurrMax = -1;
	int i;
	va_list _Valist;
	struct Constraint** _List = (struct Constraint**) malloc(sizeof(struct Constraint) * (_Size + 1));

	va_start(_Valist, _Size + sizeof(int));
	_CurrMin = va_arg(_Valist, int);
	_CurrMax = va_arg(_Valist, int);
	for(i = 0; i < _Size; ++i) {
		_List[i] = CreateConstraint(_CurrMin, _CurrMax);
		_CurrMin = _CurrMax + 1;
		_CurrMax = va_arg(_Valist, int);
	}
	_List[_Size] = NULL;
	return _List;
}

int Fuzify(struct Constraint** _List, int _Value) {
	int i;
	for(i = 0; *_List != NULL; ++i, ++_List)
		if((*_List)->Min <= _Value && (*_List)->Max >= _Value)
			return i;
	return -1;
}
