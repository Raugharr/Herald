/*
 * File: MemoryPool.h
 * Author: David Brotz
 */

#ifndef __MEMORYPOOL_H
#define __MEMORYPOOL_H

typedef struct SDL_mutex SDL_mutex;

/**
 * TODO: We should assign a memory pool to all the important and frequently used objects such as:
 * Person, Family, Crop, Good, and Population. These objects should be placed in as many pages as required,
 * however objects that are close to another such as a Good that is owned by a Family will be expected to be
 * used when the other object is used and thus they should be placed in the same page. If something happens that
 * breaks this rule like a Person who is in a completely filled page buys a Good, a grouping of objects should
 * be placed into another page.
 */

struct MemPoolNode {
	struct MemPoolNode* Next;
	struct MemPoolNode* Prev;
};

struct MemoryPool {
	struct MemPoolNode* FreeBlocks;
	struct MemPoolNode* AllocatedBlocks;
	struct MemPoolNode* BlockPool;
	int BlockSize;
#ifdef DEBUG
	int MaxSize;
	int Size;
#endif
	SDL_mutex* Lock;
};

struct MemoryPool* CreateMemoryPool(int _SizeOf, int _Quantity);
void DestroyMemoryPool(struct MemoryPool* _MemPool);
void* MemPoolAlloc(struct MemoryPool* _MemPool);
void MemPoolFree(struct MemoryPool* _MemPool, void* _Ptr);

#endif
