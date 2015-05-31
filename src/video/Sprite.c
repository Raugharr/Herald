/*
 * File: Sprite.c
 * Author: David Brotz
 */

#include "Sprite.h"

#include "Tile.h"

#include "MapRenderer.h"
#include "QuadTree.h"

#include <stdlib.h>

struct Sprite* CreateSprite(struct MapRenderer* _Renderer, SDL_Texture* _Image, int _Layer, const struct Point* _TilePos) {
	struct Sprite* _Sprite = (struct Sprite*) malloc(sizeof(struct Sprite));

	_Sprite->TilePos = *_TilePos;
	_Sprite->ScreenPos.X = _Sprite->TilePos.X * TILE_WIDTH;
	_Sprite->ScreenPos.Y = _Sprite->TilePos.Y * TILE_HEIGHT;
	QTInsertPoint(&_Renderer->RenderArea[_Layer], _Sprite, &_Sprite->TilePos);
	return _Sprite;
}

void DestroySprite(struct Sprite* _Sprite) {
	free(_Sprite);
}
