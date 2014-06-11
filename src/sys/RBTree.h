/*
 * File: RBTree.h
 * Author: David Brotz
 */

#ifndef RBTREE_H
#define RBTREE_H

struct RBNode {
	int Color;
	void* Data;
	struct RBNode* Parent;
	struct RBNode* Left;
	struct RBNode* Right;
};

struct RBItrStack{
	struct RBItrStack* Prev;
	struct RBNode* Node;
};


struct RBTree {
	struct RBNode* Table;
	int Size;
	/*
	 * Callback will return a negative number if the first void* is less than the second void*,
	 * zero if the two pointers are equal, and a positive number if the first void* is greater
	 * than the second.
	 * When Callback is called internally the user given data will always be passed to the first void*.
	 */
	int(*ICallback)(void*, void*);
	int(*SCallback)(void*, void*);
};

struct RBTree* CreateRBTree(int(*_ICallBack)(void*, void*), int(*_SCallBack)(void*, void*));
void DestroyRBTree(struct RBTree* _Tree);
void RBInsert(struct RBTree* _Tree, void* _Data);
void* RBSearch(struct RBTree* _Tree, void* _Data);
void RBDelete(struct RBTree* _Tree, void* _Data);
void RBDeleteRoot(struct RBTree* _Tree);

void RBIterate(struct RBNode* _Node, void(*_Callback)(void*));

void* RBMax(struct RBNode* _Node);
void* RBMin(struct RBNode* _Node);
int RBHeight(struct RBNode* _Node);
int RBColorCheck(struct RBNode* _Node);
int RBStrlen(struct RBNode* _Node);
int RBToString(struct RBNode* _Node, char* _Buffer, int _Size);
#endif
