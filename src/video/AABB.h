/*
 * File: AABB.h
 * Author: David Brotz
 */
#ifndef __AABB_H
#define __AABB_H

#include <SDL2/SDL.h>

typedef struct SDL_Rect SDL_Rect;

int PointInAABB(const SDL_Point* _Point, const SDL_Rect* _Rect);
int AABBInsideAABB(const SDL_Rect* _Parent, const SDL_Rect* _Inside);
int AABBIntersectsAABB(const SDL_Rect* _Parent, const SDL_Rect* _Inside);
int PointEqual(const SDL_Point* _One, const SDL_Point* _Two);
#endif
