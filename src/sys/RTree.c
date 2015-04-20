/*
 * File: RTree.c
 * Author: David Brotz
 */

#include "RTree.h"

#include "Array.h"

#include <unistd.h>

void RTreeBulkInsert(struct RTree* _Tree, struct Array* _Array, void(*BoundingBox)(const void*, int*, int*, int*, int*)) {
	int _StartX = 0;
	int _StartY = 0;
	int _EndX = 0;
	int _EndY = 0;
	int _Center = 0;
	int i = 0;
	//struct LnkLst_Node* _Itr = _List->Front;

	for(i = 0; i < _Array->Size; ++i) {
		//BoundingBox(_Itr->Data, &_StartX, &_StartY, &_EndX, &_EndY);
		_Center = (_StartX + _EndX) / 2;
	}

}
