/*
 * File: QuadTree.c
 * Author: David Brotz
 */

#include "QuadTree.h"

#include "../sys/LinkedList.h"

#include <stdlib.h>
#include <math.h>

struct QuadTree* CreateQTNode(SDL_Rect* Center) {
	struct QuadTree* Node = (struct QuadTree*) malloc(sizeof(struct QuadTree));

	Node->NorthWest = NULL;
	Node->NorthEast = NULL;
	Node->SouthEast = NULL;
	Node->SouthWest = NULL;
	Node->BoundingBox.x = Center->x;
	Node->BoundingBox.y = Center->y;
	Node->BoundingBox.w = Center->w;
	Node->BoundingBox.h = Center->h;
	Node->Data = NULL;
	return Node;
}

void QTSubdivide(struct QuadTree* Node) {
	SDL_Rect Center = {Node->BoundingBox.x, Node->BoundingBox.y, Node->BoundingBox.w / 2, Node->BoundingBox.h / 2};

	Node->NorthEast = CreateQTNode(&Center);
	Center.x = (Center.x + Center.w) + (Node->BoundingBox.w & 1);
	Node->NorthWest = CreateQTNode(&Center);

	Center.y = (Center.y + Center.h) + (Node->BoundingBox.h & 1);
	Node->SouthWest = CreateQTNode(&Center);

	Center.x = Node->BoundingBox.x;
	Node->SouthEast = CreateQTNode(&Center);
}

void QTRemoveAABB(struct QuadTree* Node, const SDL_Rect* Rect, void (*GetRect)(const void*, struct SDL_Rect*)) {
	SDL_Rect Area;

	if(Node->Data == NULL || AABBInsideAABB(Rect, &Node->BoundingBox) == 0)
		return;
	GetRect(Node->Data, &Area);
	if(Rect->x != Area.x || Rect->y != Area.y || Rect->w != Area.w || Rect->h != Area.h) {
		QTRemoveAABB(Node->NorthEast, Rect, GetRect);
		QTRemoveAABB(Node->NorthWest, Rect, GetRect);
		QTRemoveAABB(Node->SouthWest, Rect, GetRect);
		QTRemoveAABB(Node->SouthEast, Rect, GetRect);
	}
	Node->Data = NULL;
	QTMoveUp(Node);
}


int QTInsertAABB(struct QuadTree* Node, void* Data, SDL_Rect* AABB) {
	if(AABBIntersectsAABB(&Node->BoundingBox, AABB) == 0)
		return 0;
	if(Node->Data != NULL) {
		QTInsertAABB(Node->NorthEast, Data, AABB);
		QTInsertAABB(Node->NorthWest, Data, AABB);
		QTInsertAABB(Node->SouthWest, Data, AABB);
		QTInsertAABB(Node->SouthEast, Data, AABB);
		return 1;
	}
	Node->Data = Data;
	QTSubdivide(Node);
	return 1;
}

void QTRemovePoint(struct QuadTree* Node, const SDL_Point* Point, void (*GetPos)(const void*, SDL_Point*)) {
	SDL_Point Pos;

	if(Node->Data == NULL || PointInAABB(Point, &Node->BoundingBox) == 0)
		return;
	GetPos(Node->Data, &Pos);
	if(Pos.x != Point->x || Pos.y != Point->y) {
		QTRemovePoint(Node->NorthEast, Point, GetPos);
		QTRemovePoint(Node->NorthWest, Point, GetPos);
		QTRemovePoint(Node->SouthWest, Point, GetPos);
		QTRemovePoint(Node->SouthEast, Point, GetPos);
	}
	if(Pos.x == Point->x && Pos.y == Point->y) {
		Node->Data = NULL;
		QTMoveUp(Node);
	}
}


void QTRemoveNode(struct QuadTree* Node, const struct SDL_Point* Point, void (*GetPos)(const void*, SDL_Point*), void* Data) {
	SDL_Point Pos;

	if(Node->Data == NULL || PointInAABB(Point, &Node->BoundingBox) == 0)
		return;
	GetPos(Node->Data, &Pos);
//	if(Pos.x != Point->x || Pos.y != Point->y) {
		QTRemovePoint(Node->NorthEast, Point, GetPos);
		QTRemovePoint(Node->NorthWest, Point, GetPos);
		QTRemovePoint(Node->SouthWest, Point, GetPos);
		QTRemovePoint(Node->SouthEast, Point, GetPos);
//	}
	if(Node->Data == Data) {
		Node->Data = NULL;
		QTMoveUp(Node);
	}
}

int QTInsertPoint(struct QuadTree* Node, void* Data, const SDL_Point* Point) {
	if(PointInAABB(Point, &Node->BoundingBox) == 0)
		return 0;
	if(Node->Data != NULL) {
		if(QTInsertPoint(Node->NorthEast, Data, Point) == 0) {
			if(QTInsertPoint(Node->NorthWest, Data, Point) == 0) {
				if(QTInsertPoint(Node->SouthWest, Data, Point) == 0) {
					if(QTInsertPoint(Node->SouthEast, Data, Point) == 0) {
						return 0;
					}
				}
			}
		}
		return 1;
	}
	Node->Data = Data;
	if(Node->NorthEast == NULL)
		QTSubdivide(Node);
	return 1;
}

void QTPointInRectangle(const struct QuadTree* Node, const SDL_Rect* Rect, void (*GetPos)(const void*, SDL_Point*), void** Stack, uint32_t* Size, uint32_t TableSz) {
	SDL_Point Point;

	if(Node == NULL || (*Size) >= TableSz)
		return;
	if(AABBIntersectsAABB(&Node->BoundingBox, Rect) == 0)
		return;
	if(Node->Data != NULL) {
		GetPos(Node->Data, &Point);
		if(PointInAABB(&Point, Rect)) {
			Stack[(*Size)++] = Node->Data;
		}
	}
	QTPointInRectangle(Node->NorthEast, Rect, GetPos, Stack, Size, TableSz);
	QTPointInRectangle(Node->NorthWest, Rect, GetPos, Stack, Size, TableSz);
	QTPointInRectangle(Node->SouthWest, Rect, GetPos, Stack, Size, TableSz);
	QTPointInRectangle(Node->SouthEast, Rect, GetPos, Stack, Size, TableSz);
}

void* QTGetPoint(const struct QuadTree* Node, const SDL_Point* Pos, void (*GetPos)(const void*, SDL_Point*)) {
	void* Data = NULL;
	SDL_Point NodePos;

	if(Node == NULL || Node->Data == NULL || PointInAABB(Pos, &Node->BoundingBox) == 0)
		return NULL;
	GetPos(Node->Data, &NodePos);
	if(Pos->x == NodePos.x && Pos->y == NodePos.y)
		return Node->Data;
	if((Data = QTGetPoint(Node->NorthEast, Pos, GetPos)) == NULL)
		if((Data = QTGetPoint(Node->NorthWest, Pos, GetPos)) == NULL)
			if((Data = QTGetPoint(Node->SouthWest, Pos, GetPos)) == NULL)
				Data = QTGetPoint(Node->SouthEast, Pos, GetPos);
	return Data;
}

void* QTGetAABB(struct QuadTree* Node, const SDL_Point* Pos, void(*GetPos)(const void*, SDL_Rect*)) {
	void* Data = NULL;
	SDL_Rect NodePos;

	if(Node == NULL || Node->Data == NULL || PointInAABB(Pos, &Node->BoundingBox) == 0)
		return NULL;
	GetPos(Node->Data, &NodePos);
	if(PointInAABB(Pos, &NodePos) != 0)
		return Node->Data;
	if((Data = QTGetAABB(Node->NorthEast, Pos, GetPos)) == NULL)
		if((Data = QTGetAABB(Node->NorthWest, Pos, GetPos)) == NULL)
			if((Data = QTGetAABB(Node->SouthWest, Pos, GetPos)) == NULL)
				Data = QTGetAABB(Node->SouthEast, Pos, GetPos);
	return Data;
}

void QTAABBInRectangle(const struct QuadTree* Node, const SDL_Rect* Rect, void (*GetPos)(const void*, SDL_Rect*), void** Stack, uint32_t* Size, uint32_t TableSz) {
	struct SDL_Rect AABB;

	if(Node == NULL || *Size >= TableSz)
		return;
	if(AABBIntersectsAABB(&Node->BoundingBox, Rect) == 0)
		return;
	if(Node->Data != NULL) {
		GetPos(Node->Data, &AABB);
		if(AABBInsideAABB(Rect, &AABB)) {
			Stack[(*Size)++] = Node->Data;
		}
	}
	QTAABBInRectangle(Node->NorthEast, Rect, GetPos, Stack, Size, TableSz);
	QTAABBInRectangle(Node->NorthWest, Rect, GetPos, Stack, Size, TableSz);
	QTAABBInRectangle(Node->SouthWest, Rect, GetPos, Stack, Size, TableSz);
	QTAABBInRectangle(Node->SouthEast, Rect, GetPos, Stack, Size, TableSz);
}

void QTRectangleInPoint(const struct QuadTree* Node, const SDL_Point* Point, void (*GetPos)(const void*, SDL_Rect*), void** Stack, uint32_t* Size, uint32_t TableSz) {
	struct SDL_Rect AABB;

	if(Node == NULL || *Size >= TableSz)
		return;
	if(PointInAABB(Point, &Node->BoundingBox) == 0)
		return;
	if(Node->Data != NULL) {
		GetPos(Node->Data, &AABB);
		if(PointInAABB(Point, &AABB)) {
			Stack[(*Size)++] = Node->Data;
		}
	}
	QTRectangleInPoint(Node->NorthEast, Point, GetPos, Stack, Size, TableSz);
	QTRectangleInPoint(Node->NorthWest, Point, GetPos, Stack, Size, TableSz);
	QTRectangleInPoint(Node->SouthWest, Point, GetPos, Stack, Size, TableSz);
	QTRectangleInPoint(Node->SouthEast, Point, GetPos, Stack, Size, TableSz);
}

void QTMoveUp(struct QuadTree* Node) {
	if(Node->Data != NULL)
		return;
	if(Node->NorthEast->Data != NULL) {
		Node->Data = Node->NorthEast->Data;
		Node->NorthEast->Data = NULL;
		QTMoveUp(Node->NorthEast);
	} else if(Node->NorthWest->Data != NULL) {
		Node->Data = Node->NorthWest->Data;
		Node->NorthWest->Data = NULL;
		QTMoveUp(Node->NorthWest);
	} else if(Node->SouthWest->Data != NULL) {
		Node->Data = Node->SouthWest->Data;
		Node->SouthWest->Data = NULL;
		QTMoveUp(Node->SouthWest);
	} else if(Node->SouthEast->Data != NULL) {
		Node->Data = Node->SouthEast->Data;
		Node->SouthEast->Data = NULL;
		QTMoveUp(Node->SouthEast);
	}
}

void QTRange(struct QuadTree* Tree, SDL_Rect* Boundary) {
	if(AABBInsideAABB(Boundary, &Tree->BoundingBox) == 0)
		return;
}

void QTAll(const struct QuadTree* Node, void** Stack, uint32_t* Size, uint32_t TableSz) {
	if(Node == NULL || *Size >= TableSz) return;
	if(Node->Data != NULL) {
		Stack[(*Size)++] = Node->Data;
	}
	QTAll(Node->NorthEast, Stack, Size, TableSz);
	QTAll(Node->NorthWest, Stack, Size, TableSz);
	QTAll(Node->SouthWest, Stack, Size, TableSz);
	QTAll(Node->SouthEast, Stack, Size, TableSz);
}
