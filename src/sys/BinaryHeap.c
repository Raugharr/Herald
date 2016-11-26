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

	if(_Heap->Size > _Heap->TblSz) {
		//_Heap->TblSz *= 2;
		//_Heap->Table = realloc(_Heap->Table, _Heap->TblSz);
		return;
	}
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

void* BinaryHeapRemove(struct BinaryHeap* _Heap, int _Index) {
	void* _Top = _Heap->Table[0];
	void* _Temp = NULL;
	int _Left = 0;
	int _Right = 0;
	int _LeftCmp = 0;
	int _RightCmp = 0;
	int _BestChild = 0;

	if(_Index >= _Heap->Size)
		return NULL;

	_Heap->Table[_Index] = NULL;
	if(_Heap->Size == 1) {
		_Heap->Size = 0;
		return _Top;
	}
	_Heap->Table[_Index] = _Heap->Table[_Heap->Size - 1];
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

void BinaryHeapIncrease(struct BinaryHeap* _Heap, int _Index) {
	int _Min = BinaryHeapLeft(_Index);

	if(_Index >= _Heap->Size)
		return;
	while(_Min < _Heap->Size) {
		if(BinaryHeapRight(_Index) < _Heap->Size && _Heap->Compare(_Heap->Table[BinaryHeapRight(_Index)], _Heap->Table[_Min]) > 0)
			_Min = BinaryHeapRight(_Index);
		if(_Heap->Compare(_Heap->Table[_Min], _Heap->Table[_Index]) > 0) {
			void* _Temp = _Heap->Table[_Min];

			_Heap->Table[_Min] = _Heap->Table[_Index];
			_Heap->Table[_Index] = _Temp;
			_Index = _Min;
		} else {
			return;
		}
	}
}
