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
	_Center.x = (_Center.x + _Center.w) + (_Node->BoundingBox.w & 1);
	_Node->NorthWest = CreateQTNode(&_Center);

	_Center.y = (_Center.y + _Center.h) + (_Node->BoundingBox.h & 1);
	_Node->SouthWest = CreateQTNode(&_Center);

	_Center.x = _Node->BoundingBox.x;
	_Node->SouthEast = CreateQTNode(&_Center);
}

void QTRemoveAABB(struct QuadTree* _Node, const SDL_Rect* _Rect, void (*_GetRect)(const void*, struct SDL_Rect*)) {
	SDL_Rect _Area;

	if(_Node->Data == NULL || AABBInsideAABB(_Rect, &_Node->BoundingBox) == 0)
		return;
	_GetRect(_Node->Data, &_Area);
	if(_Rect->x != _Area.x || _Rect->y != _Area.y || _Rect->w != _Area.w || _Rect->h != _Area.h) {
		QTRemoveAABB(_Node->NorthEast, _Rect, _GetRect);
		QTRemoveAABB(_Node->NorthWest, _Rect, _GetRect);
		QTRemoveAABB(_Node->SouthWest, _Rect, _GetRect);
		QTRemoveAABB(_Node->SouthEast, _Rect, _GetRect);
	}
	_Node->Data = NULL;
	QTMoveUp(_Node);
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

void QTRemovePoint(struct QuadTree* _Node, const SDL_Point* _Point, void (*_GetPos)(const void*, SDL_Point*)) {
	SDL_Point _Pos;

	if(_Node->Data == NULL || PointInAABB(_Point, &_Node->BoundingBox) == 0)
		return;
	_GetPos(_Node->Data, &_Pos);
	if(_Pos.x != _Point->x || _Pos.y != _Point->y) {
		QTRemovePoint(_Node->NorthEast, _Point, _GetPos);
		QTRemovePoint(_Node->NorthWest, _Point, _GetPos);
		QTRemovePoint(_Node->SouthWest, _Point, _GetPos);
		QTRemovePoint(_Node->SouthEast, _Point, _GetPos);
	}
	_Node->Data = NULL;
	QTMoveUp(_Node);
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
	if(_Node->NorthEast == NULL)
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

void* QTGetPoint(struct QuadTree* _Node, const SDL_Point* _Pos, void (*_GetPos)(const void*, SDL_Point*)) {
	void* _Data = NULL;
	SDL_Point _NodePos;

	if(_Node == NULL || _Node->Data == NULL || PointInAABB(_Pos, &_Node->BoundingBox) == 0)
		return NULL;
	_GetPos(_Node->Data, &_NodePos);
	if(_Pos->x == _NodePos.x && _Pos->y == _NodePos.y)
		return _Node->Data;
	if((_Data = QTGetPoint(_Node->NorthEast, _Pos, _GetPos)) == NULL)
		if((_Data = QTGetPoint(_Node->NorthWest, _Pos, _GetPos)) == NULL)
			if((_Data = QTGetPoint(_Node->SouthWest, _Pos, _GetPos)) == NULL)
				_Data = QTGetPoint(_Node->SouthEast, _Pos, _GetPos);
	return _Data;
}

void* QTGetAABB(struct QuadTree* _Node, const SDL_Point* _Pos, void(*_GetPos)(const void*, SDL_Rect*)) {
	void* _Data = NULL;
	SDL_Rect _NodePos;

	if(_Node == NULL || _Node->Data == NULL || PointInAABB(_Pos, &_Node->BoundingBox) == 0)
		return NULL;
	_GetPos(_Node->Data, &_NodePos);
	if(PointInAABB(_Pos, &_NodePos) != 0)
		return _Node->Data;
	if((_Data = QTGetAABB(_Node->NorthEast, _Pos, _GetPos)) == NULL)
		if((_Data = QTGetAABB(_Node->NorthWest, _Pos, _GetPos)) == NULL)
			if((_Data = QTGetAABB(_Node->SouthWest, _Pos, _GetPos)) == NULL)
				_Data = QTGetAABB(_Node->SouthEast, _Pos, _GetPos);
	return _Data;
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

void QTRectangleInPoint(struct QuadTree* _Node, const SDL_Point* _Point, void (*_GetPos)(const void*, SDL_Rect*), struct LinkedList* _DataList) {
	struct SDL_Rect _AABB;

	if(_Node == NULL)
		return;
	if(PointInAABB(_Point, &_Node->BoundingBox) == 0)
		return;
	if(_Node->Data != NULL) {
		_GetPos(_Node->Data, &_AABB);
		if(PointInAABB(_Point, &_AABB)) {
			LnkLstPushBack(_DataList, _Node->Data);
		}
	}
	QTRectangleInPoint(_Node->NorthEast, _Point, _GetPos, _DataList);
	QTRectangleInPoint(_Node->NorthWest, _Point, _GetPos, _DataList);
	QTRectangleInPoint(_Node->SouthWest, _Point, _GetPos, _DataList);
	QTRectangleInPoint(_Node->SouthEast, _Point, _GetPos, _DataList);
}

void QTMoveUp(struct QuadTree* _Node) {
	if(_Node->Data != NULL)
		return;
	if(_Node->NorthEast->Data != NULL) {
		_Node->Data = _Node->NorthEast->Data;
		QTMoveUp(_Node->NorthEast);
	} else if(_Node->NorthWest->Data != NULL) {
		_Node->Data = _Node->NorthWest->Data;
		QTMoveUp(_Node->NorthWest);
	} else if(_Node->SouthWest->Data != NULL) {
		_Node->Data = _Node->SouthWest->Data;
		QTMoveUp(_Node->SouthWest);
	} else if(_Node->SouthEast->Data != NULL) {
		_Node->Data = _Node->SouthEast->Data;
		QTMoveUp(_Node->SouthEast);
	}
}

void QTRange(struct QuadTree* _Tree, SDL_Rect* _Boundary) {
	if(AABBInsideAABB(_Boundary, &_Tree->BoundingBox) == 0)
		return;
}
