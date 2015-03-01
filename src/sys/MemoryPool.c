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
	struct MemPoolNode* _Node = NULL;
	int _Offset = (sizeof(struct MemPoolNode) + _SizeOf) / (sizeof(int) * 2);
	struct MemPoolNode* _Last = NULL;
	int i;
	_MemPool->BlockPool = (struct MemPoolNode*) calloc(_Quantity, sizeof(struct MemPoolNode) + _SizeOf);
	_MemPool->FreeBlocks = _MemPool->BlockPool;
	_MemPool->AllocatedBlocks = NULL;
	_MemPool->NodeSize = (sizeof(struct MemPoolNode) / (sizeof(int) * 2));
#ifdef DEBUG
	memset(_MemPool->BlockPool, 0, (sizeof(struct MemPoolNode) + _SizeOf) * _Quantity);
#endif
	_MemPool->BlockSize = _SizeOf;
	_Node = _MemPool->BlockPool;

	for(i = 0; i < _Quantity; ++i) {
		_Node->Next = _Node + _Offset;
		_Node->Prev = _Node - _Offset;
		_Node = _Node->Next;
	}
	_Last = ((struct MemPoolNode*)(_MemPool->BlockPool + ((_Quantity - 1) * _Offset)));
	_Last->Next = NULL;
	_Last->Prev = _Node;
	_MemPool->BlockPool->Prev = NULL;
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
	free(_MemPool->BlockPool);
	free(_MemPool);
}
void* MemPool_Alloc(struct MemoryPool* _MemPool) {
	struct MemPoolNode* _Node = NULL;

	if(_MemPool->FreeBlocks == NULL)
		return NULL;
	_Node = _MemPool->FreeBlocks;
	_MemPool->FreeBlocks = _Node->Next;
	_Node->Prev = NULL;
	if(_MemPool->AllocatedBlocks != NULL)
		_MemPool->AllocatedBlocks->Prev = _Node;
	_Node->Next = _MemPool->AllocatedBlocks;
	_MemPool->AllocatedBlocks = _Node;
#ifdef DEBUG
	if(_MemPool->FreeBlocks < _MemPool->BlockPool && _MemPool->FreeBlocks >= (_MemPool->BlockPool + _MemPool->BlockSize * _MemPool->MaxSize))
		return NULL;
	--_MemPool->Size;
#endif
	return _Node + _MemPool->NodeSize;
}
void MemPool_Free(struct MemoryPool* _MemPool, void* _Ptr) {
	struct MemPoolNode* _Node = NULL;

#ifdef DEBUG
	if(_Ptr < (void*)_MemPool->BlockPool && _Ptr >= (void*)(_MemPool->BlockPool + _MemPool->BlockSize * _MemPool->MaxSize))
		return;
#endif
	if(_Ptr == NULL)
		return;
	_Node = _Ptr - _MemPool->NodeSize;
	if(_MemPool->FreeBlocks == NULL) {
		_MemPool->FreeBlocks = _Node;
		_Node->Next = NULL;
		_Node->Prev = NULL;
	} else {
		_Node->Prev = NULL;
		_Node->Next = _MemPool->FreeBlocks;
		_MemPool->FreeBlocks->Prev = _Node;
		_MemPool->FreeBlocks = _Node;
	}
#ifdef DEBUG
	++_MemPool->Size;
#endif
}
