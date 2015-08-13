/*
 * File: Sprite.h
 * Author: David Brotz
 */
#ifndef __SPRITE_H
#define __SPRITE_H

#include "Point.h"

#include <SDL2/SDL.h>

#define TILE_WIDTH (42)
#define TILE_WIDTH_THIRD (TILE_WIDTH * 0.75)
#define TILE_HEIGHT (48)
#define TILE_HEIGHT_THIRD (TILE_HEIGHT * 0.75)
#define TILE_GRADIENT ((TILE_HEIGHT / 4) / (TILE_WIDTH / 2))

struct MapRenderer;
typedef struct SDL_Texture SDL_Texture;

struct Sprite {
	SDL_Texture* Image;
	SDL_Point TilePos;
	SDL_Point ScreenPos;
};

struct Sprite* CreateSprite(struct MapRenderer* _Renderer, SDL_Texture* _Image, int _Layer, const SDL_Point* _TilePos);
struct Sprite* ConstructSprite(struct Sprite* _Sprite, struct MapRenderer* _Renderer, SDL_Texture* _Image, int _Layer, const SDL_Point* _TilePos);
void DestroySprite(struct Sprite* _Sprite);

static inline void SpriteGetScreenPos(const struct Sprite* _Sprite, SDL_Point* _Pos) {
	*_Pos = _Sprite->ScreenPos;
}

static inline void SpriteGetTilePos(const struct Sprite* _Sprite, SDL_Point* _Pos) {
	*_Pos = _Sprite->TilePos;
}

#endif
