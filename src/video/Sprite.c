/*
 * File: Sprite.c
 * Author: David Brotz
 */

#include "Sprite.h"

#include "Tile.h"

#include "MapRenderer.h"
#include "QuadTree.h"

#include <stdlib.h>

struct Sprite* CreateSprite(struct MapRenderer* _Renderer, SDL_Texture* _Image, int _Layer, const SDL_Point* _TilePos) {
	struct Sprite* _Sprite = (struct Sprite*) malloc(sizeof(struct Sprite));

	return ConstructSprite(_Sprite, _Renderer, _Image, _Layer, _TilePos);
}

struct Sprite* ConstructSprite(struct Sprite* _Sprite, struct MapRenderer* _Renderer, SDL_Texture* _Image, int _Layer, const SDL_Point* _TilePos) {
	_Sprite->Image = _Image;
	_Sprite->TilePos = *_TilePos;
	_Sprite->ScreenPos.x = _Sprite->TilePos.x * TILE_WIDTH;
	_Sprite->ScreenPos.y = _Sprite->TilePos.y * TILE_HEIGHT;
	QTInsertPoint(&_Renderer->RenderArea[_Layer], _Sprite, &_Sprite->TilePos);
	return _Sprite;
}

void DestroySprite(struct Sprite* _Sprite) {
	free(_Sprite);
}
