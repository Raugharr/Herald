/*
 * File: Array.c
 * Author: David Brotz
 */

#include "Array.h"

#include <stdlib.h>
#include <string.h>

struct Array* CreateArray(int _Size) {
	struct Array* _Array = (struct Array*) malloc(sizeof(struct Array));

	_Array->Table = (_Size == 0) ? (NULL) : (calloc(_Size, sizeof(void*)));
	_Array->TblSize = _Size;
	_Array->Size = 0;
	return _Array;
}

struct Array* CopyArray(const struct Array* _Array) {
	struct Array* _New = CreateArray(_Array->Size);
	int i;

	for(i = 0; i < _Array->Size; ++i)
		_New->Table[i] = _Array->Table[i];
	_New->Size = _Array->Size;
	return _New;
}

void DestroyArray(struct Array* _Array) {
	free(_Array);
}

int ArrayInsert(struct Array* _Array, void* _Data) {
	if(_Array->Size >= _Array->TblSize)
		return 0;
	_Array->Table[_Array->Size++] = _Data;
	return 1;
}

void ArrayInsert_S(struct Array* _Array, void* _Data) {
	if(_Array->Size >= _Array->TblSize) {
		ArrayResize(_Array);
	}
	_Array->Table[_Array->Size++] = _Data;
}

void ArrayRemove(struct Array* _Array, int _Index) {
	if(_Index < 0 || _Index >= _Array->TblSize)
		return;
	_Array->Table[_Index] = _Array->Table[_Array->Size - 1];
	_Array->Table[_Array->Size - 1] = NULL;
	--_Array->Size;
}

void ArrayResize(struct Array* _Array) {
	int _Size = 0;
	void* _Temp = NULL;
	
	if(_Array->Table == NULL) {
		_Size = 8;	
		_Temp = calloc(_Size, sizeof(void*));
	} else {
		_Size = _Array->TblSize * 2;
		_Temp = realloc(_Array->Table, _Size * sizeof(void*));
	}

	if(_Temp == NULL)
		return;
	//free(_Array->Table);
	_Array->Table = _Temp;
	_Array->TblSize = _Size;
}

void InsertionSort(void* _Table, int _Count, int(*_Callback)(const void*, const void*)) {
	int i;
	int j;
	int* _Temp;
	int** _Off;

	if(_Count <= 1)
		return;
	for(i = 1; i < _Count; ++i) {
		_Temp = *(int**)(_Table + sizeof(int*) * i);
		j = i;
		while(j > 0 && _Callback(_Temp, *(void**)(_Table + sizeof(int*) * (j - 1))) < 0) {
			_Off = _Table + sizeof(int*) * j;
			*_Off = *(int**)(_Table + sizeof(int*) * (j - 1));
			--j;
		}
		_Off = _Table + sizeof(int*) * j;
		*_Off = _Temp;
	}
}

void QuickSort_Aux(void* _Table, int(*_Callback)(const void*, const void*), int _Left, int _Right) {
	int i = _Left;
	int j = _Right;
	const void* _Node = NULL;
	const void** _Swap = NULL;

	if(_Left >= _Right)
		return;
	_Node = *(const void**)(_Table + sizeof(int*) * (_Left));
	do {
		while(_Callback(*((const void**)(_Table + sizeof(int*) * j)), _Node) >= 0 && i < j)
			--j;
		if(i != j) {
			_Swap = (const void**)(_Table + sizeof(int*) * i);
			*_Swap = *(const void**)(_Table + sizeof(int*) * j);
			++i;
		}
		while(_Callback(*((const void**)(_Table + sizeof(int*) * i)), _Node) <= 0 && i < j)
			++i;
		if(i != j) {
			_Swap = (const void**)(_Table + sizeof(int*) * j);
			*_Swap = *(const void**)(_Table + sizeof(int*) * i);
			--j;
		}
	} while(i < j);
	_Swap = (const void**)(_Table + sizeof(int*) * i);
	*_Swap = _Node;
 	QuickSort_Aux(_Table, _Callback, _Left, i - 1);
	QuickSort_Aux(_Table, _Callback, i + 1, _Right);
}

int ArrayLen(const void* _Table) {
	int _Size = 0;

	if(_Table == NULL)
		return 0;

	while(*((void**)_Table) != NULL) {
		_Table += sizeof(void*);
		++_Size;
	}
	return _Size;
}

void* BinarySearch(const void* _Data, void* _Table, int _Size, int(*_Callback)(const void*, const void*)) {
	int _Min = 0;
	int _Max = _Size - 1;
	int _Mid = 0;
	int _Result = 0;

	while(_Max >= _Min) {
		_Mid = _Min + ((_Max - _Min) / 2);
		_Result = _Callback(_Data, *(void**)(_Table + sizeof(void*) * _Mid));
		if(_Result < 0)
			_Max = _Mid - 1;
		else if(_Result > 0)
			_Min = _Mid + 1;
		else
			return *(void**)(_Table + sizeof(void*) * _Mid);
	}
	return NULL;
}

void* LinearSearch(const void* _Data, void* _Table, int _Size, int(*_Callback)(const void*, const void*)) {
	int i;

	for(i = 0; i < _Size; ++i) {
		if(_Callback(_Data, *(void**)_Table) == 0)
			return *(void**)_Table;
		_Table = (void**)(_Table + sizeof(void*));
	}
	return NULL;
}
