/*
 * File: KDTree.h
 * Author: David Brotz
 */
#ifndef __KDTREE_H
#define __KDTREE_H

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

#endif
