/*
 * File: RBTree.h
 * Author: David Brotz
 */

#ifndef RBTREE_H
#define RBTREE_H

struct RBTree {
	void** Table;
	int TblSize;
	int Size;
};

void RBTree_Insert(struct RBTree* _Tree, void* _Data);

#endif
