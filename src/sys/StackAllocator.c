/*
 * File: StackAllocator.c
 * Author: David Brotz
 */

#include "StackAllocator.h"

struct StackAllocator g_StackAllocator = {
		{NULL, NULL},
		{NULL, NULL},
		0,
		0
};

void* StackAllocNew(size_t _Size) {
	void* _Block = NULL;

	if((g_StackAllocator.AllocPtr[g_StackAllocator.SelSide] + _Size) >= g_StackAllocator.AllocPtr[((~g_StackAllocator.SelSide) & 1)])
		return NULL;
	_Block = g_StackAllocator.AllocPtr[g_StackAllocator.SelSide];
	g_StackAllocator.AllocPtr[g_StackAllocator.SelSide] = g_StackAllocator.AllocPtr[g_StackAllocator.SelSide] + _Size;
	return _Block;
}
