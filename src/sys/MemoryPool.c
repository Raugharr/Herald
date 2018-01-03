/*
 * File: MemoryPool.c
 * Author: David Brotz
 */

#include "MemoryPool.h"

#include "Log.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>

#define MEMPOOL_BLOCKSZ(SizeOf) ((SizeOf) * (sizeof(char) * 8))

struct MemoryPool* CreateMemoryPool(int SizeOf, int Quantity) {
	struct MemoryPool* MemPool = (struct MemoryPool*) malloc(sizeof(struct MemoryPool));

	CtorMemoryPool(MemPool, SizeOf, Quantity);
	return MemPool;
}

void CtorMemoryPool(struct MemoryPool* MemPool, int SizeOf, int Quantity) {
	struct MemPoolNode* Node = NULL;
	//int Offset = ceil((sizeof(struct MemPoolNode) + SizeOf) / ((double)8));
	intptr_t Offset = sizeof(struct MemPoolNode) + SizeOf;
	struct MemPoolNode* Last = NULL;

	MemPool->BlockPool = (struct MemPoolNode*) calloc(Quantity, MEMPOOL_BLOCKSZ(Offset));
	MemPool->FreeBlocks = MemPool->BlockPool;
	MemPool->AllocatedBlocks = NULL;
	MemPool->Lock = SDL_CreateMutex();
#ifdef DEBUG
	memset(MemPool->BlockPool, 0, (MEMPOOL_BLOCKSZ(Offset) * Quantity));
#endif
	MemPool->BlockSize = SizeOf;
	Node = MemPool->BlockPool;

	for(int i = 0; i < Quantity; ++i) {
		Node->Next = (void*)((intptr_t)Node + Offset);
		Node = Node->Next;
	}
	Last = ((struct MemPoolNode*)(MemPool->BlockPool + ((Quantity - 1) * Offset)));
	Last->Next = NULL;
#ifdef DEBUG
	MemPool->MaxSize = Quantity;
	MemPool->Size = MemPool->MaxSize;
#endif
}

void DestroyMemoryPool(struct MemoryPool* MemPool) {
#ifdef DEBUG
	//if(MemPool != NULL)
	//	Assert(MemPool->MaxSize == MemPool->Size);
#endif
	free(MemPool->BlockPool);
	SDL_DestroyMutex(MemPool->Lock);
	free(MemPool);
}
void* MemPoolAlloc(struct MemoryPool* MemPool) {
	struct MemPoolNode* Node = NULL;

	SDL_LockMutex(MemPool->Lock);
	if(MemPool->FreeBlocks == NULL)
		return NULL;
	Node = MemPool->FreeBlocks;
	MemPool->FreeBlocks = Node->Next;
	//if(MemPool->AllocatedBlocks != NULL)
	//	MemPool->AllocatedBlocks->Prev = Node;
	Node->Next = MemPool->AllocatedBlocks;
	MemPool->AllocatedBlocks = Node;
#ifdef DEBUG
	if(MemPool->FreeBlocks < MemPool->BlockPool && MemPool->FreeBlocks >= (MemPool->BlockPool + MemPool->BlockSize * MemPool->MaxSize))
		return NULL;
	--MemPool->Size;
#endif
	SDL_UnlockMutex(MemPool->Lock);
	return (void*)((char*)Node + sizeof(struct MemPoolNode));
}
void MemPoolFree(struct MemoryPool* MemPool, void* Ptr) {
	struct MemPoolNode* Node = NULL;

	SDL_LockMutex(MemPool->Lock);
#ifdef DEBUG
	if(Ptr < (void*)MemPool->BlockPool && Ptr >= (void*)(MemPool->BlockPool + MemPool->BlockSize * MemPool->MaxSize))
		return;
#endif
	if(Ptr == NULL)
		return;
	Node = Ptr - sizeof(struct MemPoolNode);
	Node->Next = MemPool->FreeBlocks;
	MemPool->FreeBlocks = Node;
#ifdef DEBUG
	++MemPool->Size;
#endif
	SDL_UnlockMutex(MemPool->Lock);
}
