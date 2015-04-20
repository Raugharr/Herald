/*
 * File: RTree.h
 * Author: David Brotz
 */
#ifndef __RTREE_H
#define __RTREE_H

struct Array;

struct RTreeNode {
	int IsLeaf;
	int StartX;
	int StartY;
	int EndX;
	int EndY;
	int ChildrenSz;
};

struct RTreeLeaf {
	int IsLeaf;
	int StartX;
	int StartY;
	int EndX;
	int EndY;
	void* Data;
};

struct RTree {
	struct RTreeNode** Block;
	int Size;
	int NodesPerPage;
};

void RTreeBulkInsert(struct RTree* _Tree, struct Array* _Array, void(*BoundingBox)(const void*, int*, int*, int*, int*));

#endif
