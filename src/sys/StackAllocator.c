/*
 * File: StackAllocator.c
 * Author: David Brotz
 */

#include "StackAllocator.h"

#include <SDL2/SDL.h>

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

void* LifoAlloc(struct LifoAllocator* _Alloc, size_t _Size) {
	void* _AllocZone = NULL;

	if((_Alloc->ArenaTop - _Alloc->ArenaBot) + (void*) _Size > (_Alloc->ArenaBot + _Alloc->ArenaSize))
		return NULL;
	_AllocZone = _Alloc->ArenaTop;
	_Alloc->ArenaTop += _Size;
	return _AllocZone;
}

void LifoFree(struct LifoAllocator* _Alloc, size_t _Size) {
	_Alloc->ArenaBot = (void*) (_Alloc->ArenaBot - _Size);
	SDL_assert(_Alloc->ArenaBot >= _Alloc->ArenaTop);

}
