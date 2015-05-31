/*
 * File: MapRenderer.c
 * Author: David Brotz
 */

#include "MapRenderer.h"

#include "Sprite.h"
#include "Tile.h"
#include "Point.h"
#include "Video.h"
#include "../sys/Log.h"
#include "../sys/LinkedList.h"

#include <SDL2/SDL_image.h>
#include <math.h>
#include <stdlib.h>

#define GetTile(_Map, _Pos) (_Map)->Tiles[(_Pos)->X + ((_Map)->TileLength * (_Pos)->Y)]

static struct Point g_TileEvenOffsets[] = {
		{-1, -1},
		{0, -1},
		{1, 0},
		{0, 1},
		{1, 1},
		{-1, 0}
};

static struct Point g_TileOddOffsets[] = {
		{0, -1},
		{1, -1},
		{1, 0},
		{1, 1},
		{0, 1},
		{-1, 0}
};

struct MapRenderer* CreateMapRenderer(int _MapLength, struct Point* _RenderSize) {
	struct MapRenderer* _Map = (struct MapRenderer*) malloc(sizeof(struct MapRenderer));

	_Map->OddGrass = IMG_LoadTexture(g_Renderer, "data/graphics/grass.png");
	_Map->Grass = IMG_LoadTexture(g_Renderer, "data/graphics/grass2.png");
	_Map->Selector = IMG_LoadTexture(g_Renderer, "data/graphics/select.png");
	_Map->Settlement = IMG_LoadTexture(g_Renderer, "data/graphics/settlement.png");

	if(_Map->Grass == NULL)
		Log(ELOG_WARNING, SDL_GetError());

	_Map->TileArea = _MapLength * _MapLength;
	_Map->TileLength = _MapLength;
	_Map->Tiles = calloc(_Map->TileArea, sizeof(struct Tile*));
	_Map->IsRendering = 0;
	for(int i = 0; i < MAPRENDER_LAYERS; ++i) {
		_Map->RenderArea[i].NorthEast = NULL;
		_Map->RenderArea[i].NorthWest = NULL;
		_Map->RenderArea[i].SouthWest = NULL;
		_Map->RenderArea[i].SouthEast = NULL;
		_Map->RenderArea[i].Data = NULL;
		_Map->RenderArea[i].BoundingBox.Center.X = _Map->TileLength / 2 * TILE_WIDTH;
		_Map->RenderArea[i].BoundingBox.Center.Y = _Map->TileLength / 2 * TILE_HEIGHT;
		_Map->RenderArea[i].BoundingBox.HalfDimension.X = _Map->RenderArea[i].BoundingBox.Center.X;
		_Map->RenderArea[i].BoundingBox.HalfDimension.Y = _Map->RenderArea[i].BoundingBox.Center.Y;
	}
	_Map->Screen.Center.X = _RenderSize->X;
	_Map->Screen.Center.Y = _RenderSize->Y;
	_Map->Screen.HalfDimension.X = _RenderSize->X;
	_Map->Screen.HalfDimension.Y = _RenderSize->Y;
	MapLoad(_Map);
	return _Map;
}

void DestroyMapRenderer(struct MapRenderer* _Map) {
	SDL_DestroyTexture(_Map->Grass);
	SDL_DestroyTexture(_Map->OddGrass);
	SDL_DestroyTexture(_Map->Selector);
	SDL_DestroyTexture(_Map->Settlement);
}

void MapLoad(struct MapRenderer* _Map) {
	int x = 0;
	int y = 0;

	for(y = 0; y < _Map->TileLength; ++y)
		for(x = 0; x < _Map->TileLength; ++x)
			_Map->Tiles[x + (y * _Map->TileLength)] = CreateTile(_Map, _Map->Grass, x, y);
}

struct Tile* ScreenToTile(struct MapRenderer* _Map, struct Point* _Screen) {
	struct Point _Hex = {0, 0};
	struct Point _RelPos;
	int _OddCol = 0;

	_Hex.Y = _Screen->Y / TILE_HEIGHT_THIRD;
	_RelPos.Y = _Screen->Y - (_Hex.Y * TILE_HEIGHT);
	_OddCol = ((_Hex.Y & 1) == 1);
	if(_OddCol != 0) {
		_Hex.X = (_Screen->X - (TILE_WIDTH / 2)) / TILE_WIDTH;
		_RelPos.X = _Screen->X - (_Hex.X * TILE_WIDTH);
	} else {
		_Hex.X = _Screen->X / TILE_WIDTH;
		_RelPos.X = _Screen->X - (_Hex.X * TILE_WIDTH) - (TILE_WIDTH / 2);
	}
	if(_RelPos.Y < 0) {
		if(_RelPos.Y < (TILE_GRADIENT * _RelPos.X) + (TILE_HEIGHT / 2)) {
			//--_Hex.Y;
			//if(_OddCol != 0)
			//	++_Hex.X;
		} else if(_RelPos.Y < (TILE_GRADIENT * _RelPos.X) - (TILE_HEIGHT / 2)) {
			--_Hex.Y;
			//if(_OddCol == 0)
			//	++_Hex.X;
		}
	}
	if(_Hex.X < 0 || _Hex.X > _Map->TileLength
			|| _Hex.Y < 0 || _Hex.Y > _Map->TileLength)
		return NULL;
	return GetTile(_Map, &_Hex);
}

struct Tile* GetAdjTile(struct MapRenderer* _Map, const struct Tile* _Tile, int _TileDir) {
	const struct Point* _Pos = NULL;

#ifdef DEBUG
	if(_TileDir < 0 || _TileDir > TILE_WEST) {
		Log(ELOG_ERROR, "GetAdjTile passed an invalid tile direction %i.", _TileDir);
		return NULL;
	}
#endif
	_Pos = ((_Tile->TilePos.Y & 1) == 1) ? (&g_TileOddOffsets[_TileDir]) : (&g_TileEvenOffsets[_TileDir]);

	return GetTile(_Map, _Pos);
}

void MapRender(SDL_Renderer* _Renderer, struct MapRenderer* _Map) {
	struct LinkedList _Data = {0, NULL, NULL};
	struct LnkLst_Node* _Itr = NULL;
	struct Sprite* _Sprite = NULL;
	SDL_Rect _Rect;

	QTPointInRectangle(&_Map->RenderArea[MAPRENDER_TILE], &_Map->Screen, ((const struct Point*(*)(const void*))SpriteGetTilePos), &_Data);
	_Itr = _Data.Front;
	while(_Itr != NULL) {
		_Sprite = (struct Sprite*)_Itr->Data;
		_Rect.x = _Sprite->ScreenPos.X - (_Map->Screen.Center.X - _Map->Screen.HalfDimension.X);
		_Rect.y = _Sprite->ScreenPos.Y - (_Map->Screen.Center.Y - _Map->Screen.HalfDimension.Y);
		_Rect.w = TILE_WIDTH;
		_Rect.h = TILE_HEIGHT;
		if((_Sprite->TilePos.Y & 1) == 1)
			SDL_RenderCopy(g_Renderer, _Map->OddGrass, NULL, &_Rect);
		else
		SDL_RenderCopy(g_Renderer, _Map->Grass, NULL, &_Rect);
		_Itr = _Itr->Next;
	}
	LnkLstClear(&_Data);
	QTPointInRectangle(&_Map->RenderArea[MAPRENDER_UNIT], &_Map->Screen, ((const struct Point*(*)(const void*))SpriteGetTilePos), &_Data);
	_Itr = _Data.Front;
	while(_Itr != NULL) {
		_Sprite = (struct Sprite*)_Itr->Data;
		_Rect.x = _Sprite->ScreenPos.X - (_Map->Screen.Center.X - _Map->Screen.HalfDimension.X);
		_Rect.y = _Sprite->ScreenPos.Y - (_Map->Screen.Center.Y - _Map->Screen.HalfDimension.Y);
		_Rect.w = TILE_WIDTH;
		_Rect.h = TILE_HEIGHT;
		SDL_RenderCopy(g_Renderer, _Map->Settlement, NULL, &_Rect);
		_Itr = _Itr->Next;
	}
	LnkLstClear(&_Data);
}
