/*
 * File: StackAllocator.c
 * Author: David Brotz
 */

#include "StackAllocator.h"

void* StackAllocNew(struct StackAllocator* _Alloc, size_t _Size) {
	void* _Block = NULL;

	if((_Alloc.AllocPtr[_Alloc.SelSide] + _Size) >= _Alloc.AllocPtr[((~_Alloc.SelSide) & 1)])
		return NULL;
	_Block = _Alloc.AllocPtr[_Alloc.SelSide];
	_Alloc.AllocPtr[_Alloc.SelSide] = _Alloc.AllocPtr[_Alloc.SelSide] + _Size;
	return _Block;
}

void StackAllocFree(struct StackAllocator* _Alloc, size_t _Size) {
	if(_Alloc.SelSide == STACKALLOC_LEFT) {
		if(_Alloc->AllocPtr[_Alloc.SelSide] - _Size < _Alloc->AllocSide[_Alloc.SelSide]) {
			_Alloc->AllocPtr[_Alloc.SelSide] = _Alloc->AllocPtr[_Alloc.SelSide] - _Size;
			return;
		}
		_Alloc->AllocPtr[_Alloc.SelSide] = _Alloc->AllocPtr[_Alloc.SelSide] - _Size;
	} else {
		if(_Alloc->AllocPtr[_Alloc.SelSide] - _Size < _Alloc->AllocSide[_Alloc.SelSide]) {
			_Alloc->AllocPtr[_Alloc.SelSide] = _Alloc->AllocPtr[_Alloc.SelSide] - _Size;
			return;
		}
		_Alloc->AllocPtr[_Alloc.SelSide] = _Alloc->AllocPtr[_Alloc.SelSide] + _Size;
	}

}
