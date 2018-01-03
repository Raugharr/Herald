/*
 * Author: David Brotz
 * File: Int.h 
*/

#ifndef __ITREE_H
#define __ITREE_H

#include "RBTree.h"

#include <stdint.h>

struct IntTreeNode {
	struct RBNode Node;
	uint32_t Key;
};

struct IntTree {
	struct IntTreeNode* Table;
	uint32_t Size;
};

struct IntTree* CreateIntTree();
void DestroyInt(struct IntTree* _Tree);
void IntInsert(struct IntTree* _Tree, uint32_t _Key, void* _Value);
void* IntSearch(const struct IntTree* _Tree, uint32_t _Key);
struct IntTreeNode* IntSearchNode(const struct IntTree* _Tree, uint32_t _Key);
void IntDelete(struct IntTree* _Tree, uint32_t _Key);
void IntDeleteNode(struct IntTree* _Tree, struct IntTreeNode* _OldNode);

#endif

