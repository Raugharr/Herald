/*
 * File: QuadTree.h
 * Author: David Brotz
 */
#ifndef __QUADTREE_H
#define __QUADTREE_H

#include "../video/AABB.h"

struct LinkedList;

struct QuadTree {
	struct QuadTree* NorthEast;
	struct QuadTree* NorthWest;
	struct QuadTree* SouthWest;
	struct QuadTree* SouthEast;
	struct AABB BoundingBox;
	void* Data;
};

void QTSubdivide(struct QuadTree* _Node);
int QTInsertAABB(struct QuadTree* _Node, void* _Data, struct AABB* _AABB);
int QTInsertPoint(struct QuadTree* _Node, void* _Data, struct Point* _Point);
void QTPointInRectangle(struct QuadTree* _Node, const struct AABB* _Rect, const struct Point* (*_GetPos)(const void*), struct LinkedList* _DataList);

#endif
