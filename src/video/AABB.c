/*
 * File: AABB.c
 * Author: David Brotz
 */

#include "AABB.h"

int PointInAABB(const struct Point* _Point, const struct AABB* _AABB) {
	return (_Point->X >= (_AABB->Center.X - _AABB->HalfDimension.X) && _Point->X <= (_AABB->Center.X + _AABB->HalfDimension.X)
			&& _Point->Y >= (_AABB->Center.Y - _AABB->HalfDimension.Y) && _Point->Y <= (_AABB->Center.Y + _AABB->HalfDimension.Y));
}

int AABBInsideAABB(const struct AABB* _Parent, const struct AABB* _Inside) {
	return ((_Inside->Center.X - _Inside->HalfDimension.X) >= (_Parent->Center.X - _Parent->HalfDimension.X) && (_Inside->Center.X + _Inside->HalfDimension.X) <= (_Parent->Center.X + _Parent->HalfDimension.X)
			&& (_Inside->Center.Y - _Inside->HalfDimension.Y) >= (_Parent->Center.Y - _Parent->HalfDimension.Y) && (_Inside->Center.Y + _Inside->HalfDimension.Y) <= (_Parent->Center.Y + _Parent->HalfDimension.Y));
}

int AABBIntersectsAABB(const struct AABB* _Parent, const struct AABB* _Inside) {
	return ((_Parent->Center.X - _Parent->HalfDimension.X) <= (_Inside->Center.X + _Inside->HalfDimension.X) && ((_Parent->Center.X + _Parent->HalfDimension.X) >= (_Inside->Center.X - _Inside->HalfDimension.X))
			&& (_Parent->Center.Y - _Parent->HalfDimension.Y) <= (_Inside->Center.Y + _Inside->HalfDimension.Y) && ((_Parent->Center.Y + _Parent->HalfDimension.Y) >= (_Inside->Center.Y - _Inside->HalfDimension.Y)));
}
