/*
 * File: Sprite.h
 * Author: David Brotz
 */
#ifndef __SPRITE_H
#define __SPRITE_H

typedef struct SDL_Texture SDL_Texture;

#include "Point.h"

struct Sprite {
	SDL_Texture* Image;
	struct Point TilePos;
	struct Point ScreenPos;
};

#endif
