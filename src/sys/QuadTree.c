/*
 * File: QuadTree.c
 * Author: David Brotz
 */

#include "QuadTree.h"

#include "LinkedList.h"

#include <stdlib.h>
#include <math.h>

struct QuadTree* CreateQTNode(struct AABB* _Center) {
	struct QuadTree* _Node = (struct QuadTree*) malloc(sizeof(struct QuadTree));

	_Node->NorthWest = NULL;
	_Node->NorthEast = NULL;
	_Node->SouthEast = NULL;
	_Node->SouthWest = NULL;
	_Node->BoundingBox.Center.X = _Center->Center.X;
	_Node->BoundingBox.Center.Y = _Center->Center.Y;
	_Node->BoundingBox.HalfDimension.X = _Center->HalfDimension.X;
	_Node->BoundingBox.HalfDimension.Y = _Center->HalfDimension.Y;
	_Node->Data = NULL;
	return _Node;
}

void QTSubdivide(struct QuadTree* _Node) {
	/*float _XCenter = ((float)_Node->BoundingBox.Center.X) - (((float)_Node->BoundingBox.HalfDimension.X) / 2);
	float _NWXCenter = _Node->BoundingBox.Center.X + (((float)_Node->BoundingBox.HalfDimension.X) / 2);
	float _YCenter = ((float)_Node->BoundingBox.Center.Y) - (((float)_Node->BoundingBox.HalfDimension.Y) / 2);
	float _HalfX = ((float)_Node->BoundingBox.HalfDimension.X) / 2;
	float _HalfY = ((float)_Node->BoundingBox.HalfDimension.Y) / 2;
	struct AABB _Center = {{floor(_XCenter),
			floor(_YCenter)},
			{floor(_HalfX), floor(_HalfY)}};
	int _OldX = _Node->BoundingBox.Center.X;

	_Node->NorthEast = CreateQTNode(&_Center);
	_Center.Center.X = ceil(_NWXCenter);
	_Center.HalfDimension.X = ceil(_HalfX);
	_Center.HalfDimension.Y = ceil(_HalfY);
	_Node->NorthWest = CreateQTNode(&_Center);
	_Center.Center.Y = ceil(_NWXCenter);
	_Node->SouthWest = CreateQTNode(&_Center);
	_Center.Center.X = _OldX;
	_Node->SouthEast = CreateQTNode(&_Center);*/
	int _OldX = _Node->BoundingBox.Center.X - (_Node->BoundingBox.HalfDimension.X / 2);
	struct AABB _Center = {{_OldX, _Node->BoundingBox.Center.Y - (_Node->BoundingBox.HalfDimension.Y / 2)}, {0, 0}};

		_Center.HalfDimension = _Center.Center;
		_Node->NorthEast = CreateQTNode(&_Center);

		_Center.Center.X = _Node->BoundingBox.Center.X + (_Node->BoundingBox.HalfDimension.X / 2);
		_Center.HalfDimension.X = _Center.Center.X;
		_Node->NorthWest = CreateQTNode(&_Center);

		_Center.Center.Y = _Node->BoundingBox.Center.Y + (_Node->BoundingBox.HalfDimension.Y / 2);
		_Center.HalfDimension.Y = _Center.Center.Y - _Node->BoundingBox.Center.Y;
		_Node->SouthWest = CreateQTNode(&_Center);

		_Center.Center.X = _OldX;
		_Node->SouthEast = CreateQTNode(&_Center);
}

int QTInsert(struct QuadTree* _Node, void* _Data, struct Point* _Point) {
	if(PointInAABB(_Point, &_Node->BoundingBox) == 0)
		return 0;
	if(_Node->Data != NULL) {
		if(QTInsert(_Node->NorthEast, _Data, _Point) == 0) {
			if(QTInsert(_Node->NorthWest, _Data, _Point) == 0) {
				if(QTInsert(_Node->SouthWest, _Data, _Point) == 0) {
					if(QTInsert(_Node->SouthEast, _Data, _Point) == 0) {
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

void QTInRectangle(struct QuadTree* _Node, const struct AABB* _Rect, const struct Point* (*_GetPos)(const void*), struct LinkedList* _DataList) {
	const struct Point* _Point = NULL;

	if(_Node == NULL)
		return;
	if(AABBIntersectsAABB(&_Node->BoundingBox, _Rect) == 0)
		return;
	if(_Node->Data != NULL) {
		_Point = _GetPos(_Node->Data);
		if(PointInAABB(_Point, _Rect)) {
			LnkLstPushBack(_DataList, _Node->Data);
		}
	}
	QTInRectangle(_Node->NorthEast, _Rect, _GetPos, _DataList);
	QTInRectangle(_Node->NorthWest, _Rect, _GetPos, _DataList);
	QTInRectangle(_Node->SouthWest, _Rect, _GetPos, _DataList);
	QTInRectangle(_Node->SouthEast, _Rect, _GetPos, _DataList);
}

void QTRange(struct QuadTree* _Tree, struct AABB* _Boundary) {
	if(AABBInsideAABB(_Boundary, &_Tree->BoundingBox) == 0)
		return;
}
