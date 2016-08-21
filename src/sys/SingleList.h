/**
 * Author: David Brotz
 * File: SingleList.h
 */

#ifndef __SINGLELIST_H
#define __SINGLELIST_H

typedef struct SDL_atomic_t SDL_atomic_t;

struct SingleLnkLst_Node {
	void* Data;
	struct SingleLnkLst_Node* Next;
};

/**
 *\struct SingleLnkLst
 * Single linked list that is lock free.
 */
struct SingleLnkLst {
	struct SingleLnkLst_Node* Front;
	struct SingleLnkLst_Node* Back;
	SDL_atomic_t _Size;
};

struct SingleList* CreateSingleLnkLst();
void DestroySingleLnkLst();

void SingleLnkLstPushBack(struct SingleLnkLst* _List, void* _Data);
void SingleLnkLstPushFront(struct SingleLnkLst* _List, void* _Data);

#endif
