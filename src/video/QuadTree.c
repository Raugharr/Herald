/*
 * File: QuadTree.c
 * Author: David Brotz
 */

#include "QuadTree.h"

#include "../sys/LinkedList.h"

#include <stdlib.h>
#include <math.h>

struct QuadTree* CreateQTNode(SDL_Rect* _Center) {
	struct QuadTree* _Node = (struct QuadTree*) malloc(sizeof(struct QuadTree));

	_Node->NorthWest = NULL;
	_Node->NorthEast = NULL;
	_Node->SouthEast = NULL;
	_Node->SouthWest = NULL;
	_Node->BoundingBox.x = _Center->x;
	_Node->BoundingBox.y = _Center->y;
	_Node->BoundingBox.w = _Center->w;
	_Node->BoundingBox.h = _Center->h;
	_Node->Data = NULL;
	return _Node;
}

void QTSubdivide(struct QuadTree* _Node) {
	SDL_Rect _Center = {_Node->BoundingBox.x, _Node->BoundingBox.y, _Node->BoundingBox.w / 2, _Node->BoundingBox.h / 2};

	_Node->NorthEast = CreateQTNode(&_Center);
	_Center.x = (_Center.x + _Center.w) + (_Node->BoundingBox.w & 1); /// 2;
	_Node->NorthWest = CreateQTNode(&_Center);

	_Center.y = (_Center.y + _Center.h) + (_Node->BoundingBox.h & 1);// / 2;
	_Node->SouthWest = CreateQTNode(&_Center);

	_Center.x = _Node->BoundingBox.x;
	_Node->SouthEast = CreateQTNode(&_Center);
}

int QTInsertAABB(struct QuadTree* _Node, void* _Data, SDL_Rect* _AABB) {
	if(AABBIntersectsAABB(&_Node->BoundingBox, _AABB) == 0)
		return 0;
	if(_Node->Data != NULL) {
		QTInsertAABB(_Node->NorthEast, _Data, _AABB);
		QTInsertAABB(_Node->NorthWest, _Data, _AABB);
		QTInsertAABB(_Node->SouthWest, _Data, _AABB);
		QTInsertAABB(_Node->SouthEast, _Data, _AABB);
		return 1;
	}
	_Node->Data = _Data;
	QTSubdivide(_Node);
	return 1;
}

int QTInsertPoint(struct QuadTree* _Node, void* _Data, const SDL_Point* _Point) {
	if(PointInAABB(_Point, &_Node->BoundingBox) == 0)
		return 0;
	if(_Node->Data != NULL) {
		if(QTInsertPoint(_Node->NorthEast, _Data, _Point) == 0) {
			if(QTInsertPoint(_Node->NorthWest, _Data, _Point) == 0) {
				if(QTInsertPoint(_Node->SouthWest, _Data, _Point) == 0) {
					if(QTInsertPoint(_Node->SouthEast, _Data, _Point) == 0) {
						return 0;
					}
				}
			}
		}
		return 1;
	}
	_Node->Data = _Data;
	QTSubdivide(_Node);
	return 1;
}

void QTPointInRectangle(struct QuadTree* _Node, const SDL_Rect* _Rect, void (*_GetPos)(const void*, SDL_Point*), struct LinkedList* _DataList) {
	SDL_Point _Point;

	if(_Node == NULL)
		return;
	if(AABBIntersectsAABB(&_Node->BoundingBox, _Rect) == 0)
		return;
	if(_Node->Data != NULL) {
		_GetPos(_Node->Data, &_Point);
		if(PointInAABB(&_Point, _Rect)) {
			LnkLstPushBack(_DataList, _Node->Data);
		}
	}
	QTPointInRectangle(_Node->NorthEast, _Rect, _GetPos, _DataList);
	QTPointInRectangle(_Node->NorthWest, _Rect, _GetPos, _DataList);
	QTPointInRectangle(_Node->SouthWest, _Rect, _GetPos, _DataList);
	QTPointInRectangle(_Node->SouthEast, _Rect, _GetPos, _DataList);
}

void QTAABBInRectangle(struct QuadTree* _Node, const SDL_Rect* _Rect, void (*_GetPos)(const void*, SDL_Rect*), struct LinkedList* _DataList) {
	struct SDL_Rect _AABB;

	if(_Node == NULL)
		return;
	if(AABBIntersectsAABB(&_Node->BoundingBox, _Rect) == 0)
		return;
	if(_Node->Data != NULL) {
		_GetPos(_Node->Data, &_AABB);
		if(AABBInsideAABB(_Rect, &_AABB)) {
			LnkLstPushBack(_DataList, _Node->Data);
		}
	}
	QTAABBInRectangle(_Node->NorthEast, _Rect, _GetPos, _DataList);
	QTAABBInRectangle(_Node->NorthWest, _Rect, _GetPos, _DataList);
	QTAABBInRectangle(_Node->SouthWest, _Rect, _GetPos, _DataList);
	QTAABBInRectangle(_Node->SouthEast, _Rect, _GetPos, _DataList);
}

void QTRange(struct QuadTree* _Tree, SDL_Rect* _Boundary) {
	if(AABBInsideAABB(_Boundary, &_Tree->BoundingBox) == 0)
		return;
}
