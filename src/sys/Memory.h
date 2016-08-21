/**
 * File: Memory.h
 * Author: David Brotz
 */

#ifndef __MEMORY_H
#define __MEMORY_H

#include "Log.h"

#include <stddef.h>
#include <stdlib.h>
#include <memory.h>

void* PageAlloc(size_t _Block);
void PageFree(void* _Block);

static inline void* Malloc(size_t _Block) {
	void* _Ptr = malloc(_Block);

	if(_Ptr == NULL) {
		Log(ELOG_ERROR, "Memory cannot be allocated.");
	}
	return _Ptr;
}

static inline void Free(void* _Ptr) {
	free(_Ptr);
}

static inline void* Realloc(void* _Ptr, size_t _Block) {
	_Ptr = realloc(_Ptr, _Block);
	if(_Ptr == NULL) {
		void* _NewPtr = Malloc(_Block);

		memcpy(_NewPtr, _Ptr, _Block);
		Free(_Ptr);
		return _NewPtr;
	}
	return _Ptr;
}

#endif
