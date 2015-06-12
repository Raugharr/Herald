/*
 * File: StackAllocator.h
 * Author: David Brotz
 */
#ifndef __STACKALLOCATOR_H
#define __STACKALLOCATOR_H

#define STACKALLOC_LEFT (0)
#define STACKALLOC_RIGHT (1)

struct StackAllocator {
	void* AllocSide[2];
	void* AllocPtr[2];
	int SelSide; /* Which side of the stack the memory will be allocated from. Use STACKALLOC_LEFT or STACKALLOC_RIGHT. */
	size_t ArenaSize;
};

void* StackAllocNew(size_t _Size);
void StackAllocFree();

#endif
