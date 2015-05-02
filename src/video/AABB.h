/*
 * File: AABB.h
 * Author: David Brotz
 */
#ifndef __AABB_H
#define __AABB_H

#include "Point.h"

struct AABB {
	struct Point Center;
	struct Point HalfDimension;
};

int PointInAABB(const struct Point* _Point, const struct AABB* _AABB);
int AABBInsideAABB(const struct AABB* _Parent, const struct AABB* _Inside);
int AABBIntersectsAABB(const struct AABB* _Parent, const struct AABB* _Inside);
#endif
