/*
 * File: MemoryPool.h
 * Author: David Brotz
 */

#ifndef __MEMORYPOOL_H
#define __MEMORYPOOL_H

struct MemoryPool {
	struct Node {struct Node* Next;}* Free;
	int SizeOf;
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
