/*
 * File: GenIterator.c
 * Author: David Brotz
 */

#include "GenIterator.h"

#include "RBTree.h"
#include "LinkedList.h"

#include <stdlib.h>

struct GenIterator* CreateRBItr(struct RBTree* _Tree, int _Size) {
	struct GenIterator* _Iterator = (struct GenIterator*) malloc(sizeof(struct GenIterator));

	_Iterator->HasNext = GenIteratorArrayHasNext;
	_Iterator->NextObj = GenIteratorArrayNextObj;
	_Iterator->StackSz = _Size;
	_Iterator->Stack = calloc(_Size, sizeof(void*));
	_Iterator->Index = 0;
	return _Iterator;
}

void DestroyRBItr(struct GenIterator* _Iterator) {
	free(_Iterator->Stack);
	free(_Iterator);
}

struct GenIterator* CreateArrayItr(void* _Array, int _Size) {
	struct GenIterator* _Iterator = (struct GenIterator*) malloc(sizeof(struct GenIterator));

	_Iterator->HasNext = GenIteratorArrayHasNext;
	_Iterator->NextObj = GenIteratorArrayNextObj;
	_Iterator->StackSz = _Size;
	_Iterator->Stack = _Array;
	_Iterator->Index = 0;
	return _Iterator;
}

void DestroyGenIterator(struct GenIterator* _Iterator) {
	free(_Iterator);
}

struct GenIterator CreateListItr(struct LinkedList* _List) {
	struct GenIterator* _Iterator = (struct GenIterator*) malloc(sizeof(struct GenIterator));

	_Iterator->HasNext = GenIteratorListHasNext;
	_Iterator->NextObj = GenIteratorListHasNext;
	_Iterator->StackSz = _List->Size;
	_Iterator->Stack = _List->Front;
	_Iterator->Index = 0;
	return _Iterator;
}

int GenIteratorArrayHasNext(struct GenIterator* _Iterator) {
	return (_Iterator->Index < _Iterator->StackSz);
}

void* GenIteratorArrayNextObj(struct GenIterator* _Iterator) {
	void* _Obj = ((void**)_Iterator->Stack)[_Iterator->Index];

	++_Iterator->Index;
	return _Obj;
}

int GenIteratorListHasNext(struct GenIterator* _Iterator) {
	return (_Iterator->Stack == NULL);
}

void* GenIteratorListHasNext(struct GenIterator* _Iterator) {
	void* _Obj = ((struct LnkLst_Node*)_Iterator->Stack)->Data;

	_Iterator->Stack = ((struct LnkLst_Node*)_Iterator->Stack)->Next;
	++_Iterator->Index;
	return _Obj;
}
