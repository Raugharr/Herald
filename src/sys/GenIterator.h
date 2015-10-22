/*
 * File: GenIterator.h
 * Author: David Brotz
 */
#ifndef __GENITERATOR_H
#define __GENITERATOR_H

/*
 * Generic container for any object that can be iterated over.
 */
struct RBTree;

struct GenIterator {
	int (*HasNext)(struct GenIterator*);
	void* (*NextObj)(struct GenIterator*);
	void* Stack;
	int Index;
	int StackSz;
};
struct GenIterator* CreateRBItr(struct RBTree* _Tree, int _Size);
void DestroyRBItr(struct GenIterator* _Iterator);

struct GenIterator* CreateArrayItr(void* _Array, int _Size);
void DestroyGenIterator(struct GenIterator* _Iterator);

struct GenIterator CreateListItr(struct LinkedList* _List);

int GenIteratorArrayHasNext(struct GenIterator* _Iterator);
void* GenIteratorArrayNextObj(struct GenIterator* _Iterator);

int GenIteratorListHasNext(struct GenIterator* _Iterator);
void* GenIteratorListNextObj(struct GenIterator* _Iterator);

#endif /* GENITERATOR_H_ */
