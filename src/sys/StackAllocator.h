/*
 * File: StackAllocator.h
 * Author: David Brotz
 */
#ifndef __STACKALLOCATOR_H
#define __STACKALLOCATOR_H

struct StackAllocator {
	void* Left;
	void* Right;
	void* LeftPtr;
	void* RightPtr;
	size_t ArenaSize;
};

#endif
