/*
 * File: Array.c
 * Author: David Brotz
 */

#include "Array.h"

#include <stdlib.h>
#include <string.h>

struct Array* CreateArray(int _Size) {
	struct Array* _Array = (struct Array*) malloc(sizeof(struct Array));

	_Array->Table = malloc(sizeof(void*) * _Size);
	_Array->TblSize = _Size;
	_Array->Size = 0;
	return _Array;
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

void ArrayResize(struct Array* _Array) {
	int _Size = _Array->TblSize * 2;
	void* _Temp = realloc(_Array->Table, _Size * sizeof(void*));

	free(_Array->Table);
	_Array->Table = _Temp;
	_Array->TblSize = _Size;
}


