/*
 * File: MemoryPool.h
 * Author: David Brotz
 */

#ifndef __MEMORYPOOL_H
#define __MEMORYPOOL_H

#define MemPoolAlloc(_MemPool) MemPool_Alloc(_MemPool)
#define MemPoolFree(_MemPool, _Ptr) MemPool_Free(_MemPool, _Ptr)

/**
 * TODO: We should assign a memory pool to all the important and frequently used objects such as:
 * Person, Family, Crop, Good, and Population. These objects should be placed in as many pages as required,
 * however objects that are close to another such as a Good that is owned by a Family will be expected to be
 * used when the other object is used and thus they should be placed in the same page. If something happens that
 * breaks this rule like a Person who is in a completely filled page buys a Good, a grouping of objects should
 * be placed into another page.
 */
struct MemoryPool {
	struct Node {
		struct Node* Next;
		struct Node* Prev;
	};
	struct Node* FreeBlocks;
	struct Node* AllocatedBlocks;
	struct Node* BlockPool;
	int BlockSize;
	int NodeSize;
#ifdef DEBUG
	int MaxSize;
	int Size;
#endif
};

struct MemoryPool* CreateMemoryPool(int _SizeOf, int _Quantity);
void DestroyMemoryPool(struct MemoryPool* _MemPool);
void* MemPool_Alloc(struct MemoryPool* _MemPool);
void MemPool_Free(struct MemoryPool* _MemPool, void* _Ptr);

#endif
