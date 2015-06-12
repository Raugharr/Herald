/*
 * File: AABB.c
 * Author: David Brotz
 */

#include "AABB.h"

#include <SDL2/SDL.h>

int PointInAABB(const SDL_Point* _Point, const SDL_Rect* _Rect) {
	return ((_Point->x >= _Rect->x) && _Point->x <= (_Rect->x + _Rect->w)
			&& _Point->y >= _Rect->y && _Point->y <= (_Rect->y + _Rect->h));
}

int AABBInsideAABB(const SDL_Rect* _Parent, const SDL_Rect* _Inside) {
	return (_Inside->x >= _Parent->x && (_Inside->x + _Inside->w) <= (_Parent->x + _Parent->w)
			&& _Inside->y >= _Parent->y && (_Inside->y + _Inside->h) <= (_Parent->y + _Parent->h));
}

int AABBIntersectsAABB(const SDL_Rect* _Parent, const SDL_Rect* _Inside) {
	return (_Parent->x <= (_Inside->x + _Inside->w) && ((_Parent->x + _Parent->w) >= (_Inside->x))
			&& (_Parent->y) <= (_Inside->y + _Inside->h) && ((_Parent->y + _Parent->h) >= (_Inside->y)));
}
