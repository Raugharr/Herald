/*
 * File: QuadTree.h
 * Author: David Brotz
 */
#ifndef __QUADTREE_H
#define __QUADTREE_H

#define QUAD_DSSIZE (512)

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

struct QuadTree* CreateQTNode(SDL_Rect* Center);
void QTSubdivide(struct QuadTree* Node);
void QTRemoveAABB(struct QuadTree* Node, const SDL_Rect* Rect, void (*_GetRect)(const void*, struct SDL_Rect*));
int QTInsertAABB(struct QuadTree* Node, void* Data, SDL_Rect* AABB);
void QTRemovePoint(struct QuadTree* Node, const SDL_Point* Point, void (*_GetPos)(const void*, SDL_Point*));
void QTRemoveNode(struct QuadTree* Node, const struct SDL_Point* Point, void (*_GetPos)(const void*, SDL_Point*), void* Data);
int QTInsertPoint(struct QuadTree* Node, void* Data, const SDL_Point* Point);
void QTPointInRectangle(const struct QuadTree* Node, const SDL_Rect* Rect, void (*_GetPos)(const void*, SDL_Point*), void** Stack, uint32_t* Size, uint32_t TableSz);

/**
 * Calls GetPos on every valid element of the QuadTree where the first argument is equal to the data and the
 * second argument set to the position of the first argument.
 */
void* QTGetPoint(const struct QuadTree* Node, const SDL_Point* Pos, void (*_GetPos)(const void*, SDL_Point*));
/**
 * Calls GetPos on every valid element of the QuadTree where the first argument is equal to the data and the
 * second argument set to the position of the first argument.
 */
void* QTGetAABB(struct QuadTree* Node, const SDL_Point* Pos, void(*_GetPos)(const void*, SDL_Rect*));

/*
 * Finds every element in range and puts every found element into Stack.\
 * \param[in] Node QuadTree node to start the search from.
 * \param[in] Rect Area to search for elements.
 * \param[in] GetPos function to determine position of elements.
 * \param[out] Stack Array that contains found elements.
 * \param[out] Size Contains how many elements were added to stack.
 * \param[in] TableSz Length of array Stack.
 */
void QTAABBInRectangle(const struct QuadTree* Node, const SDL_Rect* Rect, void (*GetPos)(const void*, struct SDL_Rect*), void** Stack, uint32_t* Size, uint32_t TableSz);
void QTRectangleInPoint(const struct QuadTree* Node, const SDL_Point* Point, void (*GetPos)(const void*, SDL_Rect*), void** Stack, uint32_t* Size, uint32_t TableSz);
void QTAll(const struct QuadTree* Node, void** Stack, uint32_t* Size, uint32_t TableSz);


/**
 * If Node's Data field is NULL will set Data to one of its children's Data
 * then sets the child's Data to NULL and calls QTMoveUp on it.
 */
void QTMoveUp(struct QuadTree* Node);

#endif
