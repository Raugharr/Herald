/*
 * File: QuadTree.h
 * Author: David Brotz
 */
#ifndef __QUADTREE_H
#define __QUADTREE_H

#include "AABB.h"

#include <SDL2/SDL.h>

struct LinkedList;

struct QuadTree {
	struct QuadTree* NorthEast;
	struct QuadTree* NorthWest;
	struct QuadTree* SouthWest;
	struct QuadTree* SouthEast;
	SDL_Rect BoundingBox;
	void* Data;
};

void QTSubdivide(struct QuadTree* _Node);
int QTInsertAABB(struct QuadTree* _Node, void* _Data, SDL_Rect* _AABB);
int QTInsertPoint(struct QuadTree* _Node, void* _Data, const SDL_Point* _Point);
void QTPointInRectangle(struct QuadTree* _Node, const SDL_Rect* _Rect, void (*_GetPos)(const void*, SDL_Point*), struct LinkedList* _DataList);
void QTAABBInRectangle(struct QuadTree* _Node, const SDL_Rect* _Rect, void (*_GetPos)(const void*, struct SDL_Rect*), struct LinkedList* _DataList);

#endif
