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

struct Constraint* CopyConstraint(struct Constraint* _Constrnt) {
	return CreateConstraint(_Constrnt->Min, _Constrnt->Max);
}

struct Constraint** CopyConstraintBnds(struct Constraint** _Constrnt) {
	int _Size = ConstrntLen(_Constrnt);
	struct Constraint** _New = calloc(_Size + 1, sizeof(struct Constraint*));
	int i;

	for(i = 0; i < _Size; ++i)
		_New[i] = CopyConstraint(_Constrnt[i]);
	_New[i] = NULL;
	return _New;
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

	if(_Size == NULL)
		return NULL;
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

int ConstrntLen(struct Constraint** _List) {
	int i;
	int _Size = 0;

	for(i = 0; _List[i] != NULL; ++i)
		++_Size;
	return _Size;
}
