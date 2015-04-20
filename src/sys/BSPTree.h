/*
 * File: BSPTree.h
 * Author: David Brotz
 */
#ifndef __BSPTREE_H
#define __BSPTREE_H

struct BSPNode {
	struct BSPNode* Left;
	struct BSPNode* Right;
	void* Data;
};

struct BSPTree {

};

#endif
