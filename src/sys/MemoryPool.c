/*
 * File: MemoryPool.c
 * Author: David Brotz
 */

#include "MemoryPool.h"

#include <stdlib.h>

#define MEMPOOL_NEXT(__MemPool, __Index) ((struct Node*)((char*)(__MemPool)->Free) + ((__Index) * ((__MemPool)->SizeOf + sizeof(struct Node))))

struct MemoryPool* CreateMemoryPool(int _SizeOf, int _Quantity) {
	struct MemoryPool* _MemPool = (struct MemoryPool*) malloc(sizeof(struct MemoryPool) * 10);
	struct Node* _Node = NULL;
	int i;

	_MemPool->Free = (struct Node*) malloc((sizeof(struct Node) + _SizeOf) * _Quantity * 10);
	_MemPool->SizeOf = _SizeOf;
	_Node = _MemPool->Free;

	for(i = 0; i < _Quantity - 1; ++i) {
		_Node->Next = _Node + (sizeof(struct Node) + _SizeOf);
		_Node = _Node->Next;
	}
	MEMPOOL_NEXT(_MemPool, _Quantity - 1)->Next = NULL;
	return _MemPool;
}
void DestroyMemoryPool(struct MemoryPool* _MemPool) {
	free(_MemPool->Free);
	free(_MemPool);
}
void* MemPool_Alloc(struct MemoryPool* _MemPool) {
	struct Node* _Node = NULL;

	if(_MemPool->Free == NULL)
		return NULL;
	_Node = _MemPool->Free;
	_MemPool->Free = _MemPool->Free->Next;
	return _Node + sizeof(struct Node);
}
void MemPool_Free(struct MemoryPool* _MemPool, void* _Ptr) {
	struct Node* _Node = _Ptr - sizeof(struct Node);

	if(_MemPool->Free == NULL) {
		_MemPool->Free = _Node;
		return;
	}
	_Node->Next = _MemPool->Free;
	_MemPool->Free = _Node;
}

