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
	int* _SizePtr = NULL;
	int _CurrMin = _Min;
	int _CurrMax = _Max;
	struct Constraint** _List = NULL;
	int i;

	if(_Size == NULL) {
		_Size = (int*) malloc(sizeof(int));
		_SizePtr = _Size;
	}
	*_Size = _Max / _Interval;
	if(_Interval * *_Size < _Max)
		++(*_Size);
	_List = (struct Constraint**) malloc(sizeof(struct Constraint) * *_Size);
	while(1) {
		for(i = 0; i < *_Size; ++i) {
			if(_CurrMax > _Max) {
				_List[i] = CreateConstraint(_CurrMin, _Max);
				break;
			}
			_List[i] = CreateConstraint(_CurrMin, _CurrMax);
			_CurrMin += _Interval;
			_CurrMax += _Interval;
		}
		break;
	}
	_List[i + 1] = NULL;
	free(_SizePtr);
	return _List;
}

struct Constraint** CreateConstrntBnds(int _Size, ...) {
	va_list _Valist;
	struct Constraint** _List = NULL;

	va_start(_Valist, _Size);
	_List = CreateConstrntVaBnds(_Size, _Valist);
	_List[_Size] = NULL;
	va_end(_Valist);
	return _List;
}

struct Constraint** CreateConstrntVaBnds(int _Size, va_list _List) {
	int _CurrMin = -1;
	int _CurrMax = -1;
	int i;
	struct Constraint** _Constrnt = (struct Constraint**) malloc(sizeof(struct Constraint) * (_Size + 1));

	_CurrMin = va_arg(_List, int);
	_CurrMax = va_arg(_List, int);
	for(i = 0; i < _Size; ++i) {
		_Constrnt[i] = CreateConstraint(_CurrMin, _CurrMax);
		_CurrMin = _CurrMax + 1;
		_CurrMax = va_arg(_List, int);
	}
	_Constrnt[_Size] = NULL;
	return _Constrnt;
}

int Fuzify(struct Constraint** _List, int _Value) {
	int i;
	for(i = 0; *_List != NULL; ++i, ++_List)
		if((*_List)->Min <= _Value && (*_List)->Max >= _Value)
			return i;
	return -1;
}
