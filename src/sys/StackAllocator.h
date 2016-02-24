/*
 * File: StackAllocator.h
 * Author: David Brotz
 */
#ifndef __STACKALLOCATOR_H
#define __STACKALLOCATOR_H

#include <stddef.h>

#define STACKALLOC_LEFT (0)
#define STACKALLOC_RIGHT (1)

struct StackAllocator {
	void* AllocSide[2];
	void* AllocPtr[2]; /*Pointer to the beginning of the stack.*/
	int SelSide; /* Which side of the stack the memory will be allocated from. Use STACKALLOC_LEFT or STACKALLOC_RIGHT. */
	size_t ArenaSize;
};

struct LifoAllocator {
	void* ArenaTop;
	void* ArenaBot;
	size_t ArenaSize;
};

void* StackAllocNew(struct StackAllocator* _Alloc, size_t _Size);
void StackAllocFree(struct StackAllocator* _Alloc, size_t _Size);

void* LifoAlloc(struct LifoAllocator* _Alloc, size_t _Size);
void LifoFree(struct LifoAllocator* _Alloc, size_t _Size);

#endif
