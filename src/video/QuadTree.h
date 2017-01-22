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
void QTRemoveAABB(struct QuadTree* _Node, const SDL_Rect* _Rect, void (*_GetRect)(const void*, struct SDL_Rect*));
int QTInsertAABB(struct QuadTree* _Node, void* _Data, SDL_Rect* _AABB);
void QTRemovePoint(struct QuadTree* _Node, const SDL_Point* _Point, void (*_GetPos)(const void*, SDL_Point*));
void QTRemoveNode(struct QuadTree* _Node, const struct SDL_Point* _Point, void (*_GetPos)(const void*, SDL_Point*), void* _Data);
int QTInsertPoint(struct QuadTree* _Node, void* _Data, const SDL_Point* _Point);
void QTPointInRectangle(struct QuadTree* _Node, const SDL_Rect* _Rect, void (*_GetPos)(const void*, SDL_Point*), struct LinkedList* _DataList);

/**
 * Calls _GetPos on every valid element of the QuadTree where the first argument is equal to the data and the
 * second argument set to the position of the first argument.
 */
void* QTGetPoint(struct QuadTree* _Node, const SDL_Point* _Pos, void (*_GetPos)(const void*, SDL_Point*));
/**
 * Calls _GetPos on every valid element of the QuadTree where the first argument is equal to the data and the
 * second argument set to the position of the first argument.
 */
void* QTGetAABB(struct QuadTree* _Node, const SDL_Point* _Pos, void(*_GetPos)(const void*, SDL_Rect*));

void QTAABBInRectangle(struct QuadTree* _Node, const SDL_Rect* _Rect, void (*_GetPos)(const void*, struct SDL_Rect*), struct LinkedList* _DataList);
void QTRectangleInPoint(struct QuadTree* _Node, const SDL_Point* _Point, void (*_GetPos)(const void*, SDL_Rect*), struct LinkedList* _DataList);


/**
 * If _Node's Data field is NULL will set Data to one of its children's Data
 * then sets the child's Data to NULL and calls QTMoveUp on it.
 */
void QTMoveUp(struct QuadTree* _Node);

#endif
