/*
 * File: BinaryHeap.c
 * Author: David Brotz
 */

#include "BinaryHeap.h"

#include <stdlib.h>

struct BinaryHeap* CreateBinaryHeap(int _Size, int (*_ICallback)(const void*, const void*)) {
	struct BinaryHeap* _Heap = (struct BinaryHeap*) malloc(sizeof(struct BinaryHeap));

	_Heap->Table = calloc(_Size, sizeof(void*));
	_Heap->TblSz = _Size;
	_Heap->Size = 0;
	_Heap->Compare = _ICallback;
	return _Heap;
}

void DestroyBinaryHeap(struct BinaryHeap* _Heap) {
	free(_Heap->Table);
	free(_Heap);
}

void BinaryHeapInsert(struct BinaryHeap* _Heap, void* _Data) {
	int _Parent = BinaryHeapParent(_Heap->Size);
	int _Index = _Heap->Size;

	if(_Heap->Size >= _Heap->TblSz)
		return;
	_Heap->Table[_Heap->Size] = _Data;
	while(_Parent != _Index) {
		if(_Heap->Compare(_Data, _Heap->Table[_Parent]) > 0) {
			void* _Temp = _Heap->Table[_Parent];

			_Heap->Table[_Parent] = _Data;
			_Heap->Table[_Index] = _Temp;
		}
		_Index = _Parent;
		_Parent = BinaryHeapParent(_Index);
	}
	++_Heap->Size;
}

void* BinaryHeapPop(struct BinaryHeap* _Heap) {
	void* _Top = _Heap->Table[0];
	void* _Temp = NULL;
	int _Index = 0;
	int _Left = 0;
	int _Right = 0;
	int _LeftCmp = 0;
	int _RightCmp = 0;
	int _BestChild = 0;

	_Heap->Table[0] = NULL;
	if(_Heap->Size == 1) {
		_Heap->Size = 0;
		return _Top;
	}
	_Heap->Table[0] = _Heap->Table[_Heap->Size - 1];
	_Heap->Table[_Heap->Size - 1] = NULL;
	--_Heap->Size;
	while(1) {
		_Left = BinaryHeapLeft(_Index);
		_Right = BinaryHeapRight(_Index);
		if(BinaryHeapIsNode(_Left, _Heap) != 0) {
			_LeftCmp = _Heap->Compare(_Heap->Table[_Index], _Heap->Table[_Left]);
			if(BinaryHeapIsNode(_Right, _Heap) != 0) {
				_RightCmp = _Heap->Compare(_Heap->Table[_Index], _Heap->Table[_Right]);
				if(_LeftCmp < _RightCmp)
					_BestChild = _Left;
				else
					_BestChild = _Right;
			} else if(_LeftCmp < 0){
				_BestChild = _Left;
			} else
				break;
		} else {
			break;
		}
		_Temp = _Heap->Table[_Index];
		_Heap->Table[_Index] = _Heap->Table[_BestChild];
		_Heap->Table[_BestChild] = _Temp;
		_Index = _BestChild;
	}

	return _Top;
}
