/*
 * File: KDTree.h
 * Author: David Brotz
 */
#ifndef __KDTREE_H
#define __KDTREE_H

struct Stack;
struct StackNode;
struct LinkedList;

#define KDRange(_Tree, _Pos, _Area) KDRange_Aux((_Tree)->Root, 0, (_Pos), (_Area), NULL)
#define KDHeight(_Tree) KDHeightNode((_Tree)->Root)

struct KDNode {
	struct KDNode* Parent;
	struct KDNode* Left;
	struct KDNode* Right;
	int Pos[2];
	void* Data;
};

struct KDTree {
	struct KDNode* Root;
	int Size;
};

struct KDTree* CreateKDTree();
void DestroyKDTree(struct KDTree* _Tree);

void KDDeleteAll(struct KDTree* _Tree);

void* KDInsertSearch(struct KDTree* _Tree, void* _Data, int _Pos[2]);
void KDInsert(struct KDTree* _Tree, void* _Data, int _X, int _Y);
struct KDNode* KDSearchNode(const struct KDTree* _Tree, int _Data[2]);
void* KDSearch(const struct KDTree* _Tree, int _Data[2]);

void KDBalance(struct KDTree* _Tree);
struct KDNode** KDToArray(struct KDNode** _List, const struct KDNode* _Node, int _Axis, int _Size);

struct LinkedList* KDRange_Aux(struct KDNode* _Node, int _Axis, int _Pos[2], int _Area[2], struct LinkedList* _List);
int KDHeightNode(struct KDNode* _Node);

struct KDNode* KDNextNode(struct KDNode* _Node);
#endif
