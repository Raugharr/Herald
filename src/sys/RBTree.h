/*
 * Author: David Brotz
 * File: RBTree.h 
*/

#ifndef RBTREE_H
#define RBTREE_H

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef int(*RBCallback)(const void*, const void*);

enum RB_COLOR {
	RB_RED,
	RB_BLACK,
	RB_DBLACK
};

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
	/**
	 * Callback will return a negative number if the first void* is less than the second void*,
	 * zero if the two pointers are equal, and a positive number if the first void* is greater
	 * than the second.
	 * When Callback is called internally the user given data will always be passed to the first void*.
	 */
	RBCallback ICallback;
	RBCallback SCallback;
};

struct RBTree* CreateRBTree(RBCallback ICallback, RBCallback SCallback);
struct RBTree* CopyRBTree(struct RBTree* _Tree);
void DestroyRBTree(struct RBTree* _Tree);
void RBBalance(struct RBTree* _Tree, struct RBNode* _Node);

void RBInsert(struct RBTree* _Tree, void* _Data);
/**
 * If _Search is found to be in the tree it will return its RBNode.
 * If _Search is not in the tree _Insert will be added and NULL will
 * be returned.
 */
struct RBNode* RBInsertSearch(struct RBTree* _Tree, void* _Search, void* _Insert);
/**
 * Searches _Tree for _Data by using _Tree's SCallback with _Data always being the first parameter.
 * Returns NULL if no value can be found or the value that _Data is the key to.
 */
void* RBSearch(const struct RBTree* _Tree, const void* _Data);
struct RBNode* RBSearchNode(const struct RBTree* _Tree, const void* _Data);
/**
 * Deletes a single node containing _Data if it exists.
 */
void RBDelete(struct RBTree* _Tree, void* _Data);
void RBDeleteNode(struct RBTree* _Tree, struct RBNode* _OldNode);

struct RBItrStack* RBDepthFirst(struct RBNode* const _Node, struct RBItrStack* _Stack);
/**
 * _Callback takes a single argument that will contain a pointer to an RBNode's Data field.
 * _Callback will remove the RBNode that contains _Callback's argument from the tree if _Callback returns 1
 * otherwise nothing will happen.
 * @return How many elements from the red black tree that have been deleted.
 */
void RBIterate(struct RBTree* _Tree, int(*_Callback)(void*));
void RBRemoveAll(struct RBTree* _Tree, void(*_Callback)(void*));
/**
 * Removes all elements from _Tree.
 */
void RBClear(struct RBTree* _Tree);
/**
 *  Returns the data of the largest element that is _Node or a child of _Node.
 */
void* RBMax(struct RBNode* _Node);
/**
 *  Returns the data of the smallest element that is _Node or a child of _Node.
 */
void* RBMin(struct RBNode* _Node);
int RBInvariant(struct RBNode* Node, RBCallback ICallback);
int RBCount(struct RBNode* Node);
/**
 * Returns the height of this tree.
*/
int RBHeight(struct RBNode* _Node);
/**
 *  Returns true if this tree is balanced.
 */
int RBColorCheck(struct RBNode* _Node);
int RBStrlen(struct RBNode* _Node);
int RBToString(struct RBNode* _Node, char* _Buffer, int _Size);
int RBRange(struct RBTree* _Tree, void* _Min, void* _Max, void** _RangeTbl, int _MaxSize);
void RBRotateLeft(struct RBNode** _Tree, struct RBNode* _Root);
void RBRotateRight(struct RBNode** _Tree, struct RBNode* _Root);
static inline struct RBNode* RBSibling(struct RBNode* _Node) {
	if(_Node->Parent == NULL)
		return NULL;
	if(_Node == _Node->Parent->Left)
		return _Node->Parent->Right;
	return _Node->Parent->Left;
}

#endif

