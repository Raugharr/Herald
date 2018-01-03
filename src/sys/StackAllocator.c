/*
 * File: StackAllocator.c
 * Author: David Brotz
 */

#include "StackAllocator.h"

#include "Log.h"

#include <stdlib.h>

void* StackAllocNew(struct StackAllocator* _Alloc, size_t _Size) {
	void* _Block = NULL;

	if((_Alloc->AllocPtr[_Alloc->SelSide] + _Size) >= _Alloc->AllocPtr[((~_Alloc->SelSide) & 1)])
		return NULL;
	_Block = _Alloc->AllocPtr[_Alloc->SelSide];
	_Alloc->AllocPtr[_Alloc->SelSide] = _Alloc->AllocPtr[_Alloc->SelSide] + _Size;
	return _Block;
}

void StackAllocFree(struct StackAllocator* _Alloc, size_t _Size) {
	if(_Alloc->SelSide == STACKALLOC_LEFT) {
		if(_Alloc->AllocPtr[_Alloc->SelSide] - _Size < _Alloc->AllocSide[_Alloc->SelSide]) {
			_Alloc->AllocPtr[_Alloc->SelSide] = _Alloc->AllocPtr[_Alloc->SelSide] - _Size;
			return;
		}
		_Alloc->AllocPtr[_Alloc->SelSide] = _Alloc->AllocPtr[_Alloc->SelSide] - _Size;
	} else {
		if(_Alloc->AllocPtr[_Alloc->SelSide] - _Size < _Alloc->AllocSide[_Alloc->SelSide]) {
			_Alloc->AllocPtr[_Alloc->SelSide] = _Alloc->AllocPtr[_Alloc->SelSide] - _Size;
			return;
		}
		_Alloc->AllocPtr[_Alloc->SelSide] = _Alloc->AllocPtr[_Alloc->SelSide] + _Size;
	}
}

void InitLifoAlloc(struct LifoAllocator* _LifoAlloc, size_t _Size) {
	_LifoAlloc->ArenaBot = malloc(_Size);
	_LifoAlloc->ArenaTop = _LifoAlloc->ArenaBot;
	_LifoAlloc->ArenaSize = _Size;
}

void* LifoAlloc(struct LifoAllocator* _Alloc, size_t _Size) {
	void* _AllocZone = NULL;

	if(((void*) (_Alloc->ArenaTop - _Alloc->ArenaBot) + _Size) > (_Alloc->ArenaBot + _Alloc->ArenaSize))
		return NULL;
	_AllocZone = _Alloc->ArenaTop;
	_Alloc->ArenaTop += _Size;
	return _AllocZone;
}

void LifoFree(struct LifoAllocator* _Alloc, size_t _Size) {
	_Alloc->ArenaTop = (void*) (_Alloc->ArenaTop - _Size);
	Assert(_Alloc->ArenaBot <= _Alloc->ArenaTop);
}
