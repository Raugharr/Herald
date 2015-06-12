/*
 * File: Tile.c
 * Author: David Brotz
 */

#include "Tile.h"

#include "MapRenderer.h"
#include "QuadTree.h"
#include "Sprite.h"

#include <SDL2/SDL.h>
#include <stdlib.h>

struct Tile* CreateTile(struct MapRenderer* _Renderer, SDL_Texture* _Image, int _X, int _Y) {
	struct Tile* _Tile = (struct Tile*) malloc(sizeof(struct Tile));

	_Tile->Image = _Image;
	_Tile->TilePos.x = _X;
	_Tile->TilePos.y = _Y;
	_Tile->ScreenPos.y = _Y * TILE_HEIGHT_THIRD;
	_Tile->ScreenPos.x = _X * TILE_WIDTH;
	//_Tile->ScreenPos.Y = _Y * (TILE_HEIGHT / 2) + (_Y * (TILE_HEIGHT / 4));
	if((_Y & 1) == 1) {
		_Tile->ScreenPos.x += (TILE_WIDTH / 2);
	}
	_Tile->Forest = 0;
	_Tile->Temperature = 0;
	_Tile->Unbuildable = 0;
	QTInsertPoint(&_Renderer->RenderArea[MAPRENDER_TILE], _Tile, &_Tile->TilePos);
	return _Tile;
}

void DestroyTile(struct Tile* _Tile) {
	free(_Tile);
}
