/*
 * File: MapRenderer.c
 * Author: David Brotz
 */

#include "MapRenderer.h"

#include "Tile.h"
#include "Point.h"
#include "Video.h"
#include "../sys/LinkedList.h"

#include <SDL2/SDL_image.h>
#include <math.h>
#include <stdlib.h>

struct MapRenderer* CreateMapRenderer(int _MapLength, struct Point* _RenderSize) {
	struct MapRenderer* _Map = (struct MapRenderer*) malloc(sizeof(struct MapRenderer));
	_Map->Grass = IMG_LoadTexture(g_Renderer, "data/graphics/grass2.png");

	_Map->TileArea = _MapLength * _MapLength;
	_Map->TileLength = _MapLength;
	_Map->Tiles = calloc(_Map->TileArea, sizeof(struct Tile*));
	_Map->RenderArea.NorthEast = NULL;
	_Map->RenderArea.NorthWest = NULL;
	_Map->RenderArea.SouthWest = NULL;
	_Map->RenderArea.SouthEast = NULL;
	_Map->RenderArea.Data = NULL;
	_Map->RenderArea.BoundingBox.Center.X = _Map->TileLength / 2 * TILE_WIDTH;
	_Map->RenderArea.BoundingBox.Center.Y = _Map->TileLength / 2 * TILE_HEIGHT;
	_Map->RenderArea.BoundingBox.HalfDimension.X = _Map->RenderArea.BoundingBox.Center.X;
	_Map->RenderArea.BoundingBox.HalfDimension.Y = _Map->RenderArea.BoundingBox.Center.Y;
	_Map->Screen.Center.X = _RenderSize->X;
	_Map->Screen.Center.Y = _RenderSize->Y;
	_Map->Screen.HalfDimension.X = _RenderSize->X;
	_Map->Screen.HalfDimension.Y = _RenderSize->Y;
	MapLoad(_Map);
	return _Map;
}

void DestroyMapRenderer(struct MapRenderer* _Map) {
	SDL_DestroyTexture(_Map->Grass);
}

void MapLoad(struct MapRenderer* _Map) {
	int x = 0;
	int y = 0;

	for(y = 0; y < _Map->TileLength; ++y)
		for(x = 0; x < _Map->TileLength; ++x)
			_Map->Tiles[x + (y * _Map->TileLength)] = CreateTile(_Map, _Map->Grass, x, y);
}

struct Tile* MapGetTile(struct MapRenderer* _Map, struct Point* _Screen) {
	struct Point _Hex = {0, 0};

	_Hex.X = (_Screen->X * sqrt(3) / 3 - _Screen->Y / 3 / TILE_WIDTH);
	_Hex.Y = _Screen->Y * 2 / 3  / TILE_WIDTH;
	return _Map->Tiles[_Hex.X * (_Map->TileLength * _Hex.Y)];
}

void MapRender(SDL_Renderer* _Renderer, struct MapRenderer* _Map) {
	struct LinkedList _Data = {0, NULL, NULL};
	struct LnkLst_Node* _Itr = NULL;
	struct Tile* _Tile = NULL;
	SDL_Rect _Rect;

	QTInRectangle(&_Map->RenderArea, &_Map->Screen, ((const struct Point*(*)(const void*))TileGetScreenPos), &_Data);
	_Itr = _Data.Front;
	while(_Itr != NULL) {
		_Tile = (struct Tile*)_Itr->Data;
		_Rect.x = _Tile->ScreenPos.X - (_Map->Screen.Center.X - _Map->Screen.HalfDimension.X);
		_Rect.y = _Tile->ScreenPos.Y - (_Map->Screen.Center.Y - _Map->Screen.HalfDimension.Y);
		_Rect.w = TILE_WIDTH;
		_Rect.h = TILE_HEIGHT;
		SDL_RenderCopy(g_Renderer, _Map->Grass, NULL, &_Rect);
		_Itr = _Itr->Next;
	}
	LnkLstClear(&_Data);
}
