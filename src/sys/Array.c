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
	memset(_Array->Table, 0, sizeof(void*) * _Size);
	_Array->TblSize = _Size;
	_Array->Size = 0;
	return _Array;
}

void DestroyArray(struct Array* _Array) {
	free(_Array);
}

int Array_Insert(struct Array* _Array, void* _Data) {
	if(_Array->Size >= _Array->TblSize)
		return 0;
	_Array->Table[_Array->Size++] = _Data;
	return 1;
}

