/*
 * File: MemoryPool.c
 * Author: David Brotz
 */

#include "MemoryPool.h"

#include <stdlib.h>
#include <string.h>
#ifdef DEBUG
#include <assert.h>
#endif

struct MemoryPool* CreateMemoryPool(int _SizeOf, int _Quantity) {
	struct MemoryPool* _MemPool = (struct MemoryPool*) malloc(sizeof(struct MemoryPool));
	struct Node* _Node = NULL;
	int _Offset = (sizeof(struct Node) + _SizeOf) / sizeof(int);
	int i;

	_MemPool->Free = (struct Node*) malloc((sizeof(struct Node) + _SizeOf) * _Quantity);
	_MemPool->NodeSize = (sizeof(struct Node) / sizeof(int));
#ifdef DEBUG
	memset(_MemPool->Free, 0, (sizeof(struct Node) + _SizeOf) * _Quantity);
#endif
	_MemPool->SizeOf = _SizeOf;
	_Node = _MemPool->Free;

	for(i = 0; i < _Quantity; ++i) {
		_Node->Next = _Node + _Offset;
		_Node = _Node->Next;
	}
	((struct Node*)(_MemPool->Free + ((_Quantity - 1) * _Offset)))->Next = NULL;
#ifdef DEBUG
	_MemPool->MaxSize = _Quantity;
	_MemPool->Size = _MemPool->MaxSize;
#endif
	return _MemPool;
}
void DestroyMemoryPool(struct MemoryPool* _MemPool) {
#ifdef DEBUG
	assert(_MemPool->MaxSize == _MemPool->Size);
#endif
	free(_MemPool->Free);
	free(_MemPool);
}
void* MemPool_Alloc(struct MemoryPool* _MemPool) {
	struct Node* _Node = NULL;

	if(_MemPool->Free == NULL)
		return NULL;
	_Node = _MemPool->Free;
	_MemPool->Free = _Node->Next;
#ifdef DEBUG
	--_MemPool->Size;
#endif
	return _Node + _MemPool->NodeSize;
}
void MemPool_Free(struct MemoryPool* _MemPool, void* _Ptr) {
	struct Node* _Node = NULL;

	if(_Ptr == NULL)
		return;
	_Node = _Ptr - _MemPool->NodeSize;
	if(_MemPool->Free == NULL) {
		_MemPool->Free = _Node;
	} else
		_Node->Next = _MemPool->Free; {
		_MemPool->Free = _Node;
	}
#ifdef DEBUG
	++_MemPool->Size;
#endif
}
