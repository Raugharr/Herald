/*
 * File: MapRenderer.c
 * Author: David Brotz
 */

#include "MapRenderer.h"

#include "Sprite.h"
#include "Tile.h"
#include "Video.h"

#include "../sys/ResourceManager.h"
#include "../sys/Math.h"
#include "../sys/Log.h"
#include "../sys/LinkedList.h"
#include "../sys/ResourceManager.h"

#include "../Warband.h"
#include "../Location.h"
#include "../World.h"

#include <SDL2/SDL_image.h>
#include <math.h>
#include <stdlib.h>

struct MapRenderer* CreateMapRenderer(int _MapLength, SDL_Point* _RenderSize) {
	struct MapRenderer* _Map = (struct MapRenderer*) malloc(sizeof(struct MapRenderer));

	_Map->Selector = ResourceGet("Select.png");
	_Map->TileSheets[0].TileFile = ResourceGet("grass.png");
	_Map->TileSheets[0].Tiles = ResourceGetData(_Map->TileSheets[0].TileFile);
	_Map->TileSheets[0].VarPos[0].x = 0;
	_Map->TileSheets[0].VarPos[0].y = 0;

	_Map->TileArea = _MapLength * _MapLength;
	_Map->TileLength = _MapLength;
	_Map->Tiles = calloc(_Map->TileArea, sizeof(struct Tile));
	_Map->IsRendering = 0;
	for(int i = 0; i < MAPRENDER_LAYERS; ++i) {
		_Map->RenderArea[i].NorthEast = NULL;
		_Map->RenderArea[i].NorthWest = NULL;
		_Map->RenderArea[i].SouthWest = NULL;
		_Map->RenderArea[i].SouthEast = NULL;
		_Map->RenderArea[i].Data = NULL;
		_Map->RenderArea[i].BoundingBox.x = 0;
		_Map->RenderArea[i].BoundingBox.y = 0;
		_Map->RenderArea[i].BoundingBox.w = _Map->TileLength * TILE_WIDTH;
		_Map->RenderArea[i].BoundingBox.h = _Map->TileLength * TILE_HEIGHT;
	}
	_Map->Screen.x = 0;
	_Map->Screen.y = 0;
	_Map->Screen.w = _RenderSize->x;
	_Map->Screen.h = _RenderSize->y;
	MapLoad(_Map);
	return _Map;
}

void DestroyMapRenderer(struct MapRenderer* _Map) {
	SDL_DestroyTexture(_Map->Grass);
	SDL_DestroyTexture(_Map->OddGrass);
	SDL_DestroyTexture(_Map->Settlement);
	SDL_DestroyTexture(_Map->Warrior);
}

void MapLoad(struct MapRenderer* _Map) {
	struct Tile* _Tile = NULL;

	for(int y = 0; y < _Map->TileLength; ++y) {
		for(int x = 0; x < _Map->TileLength; ++x) {
			_Tile->Forest = 0;
			_Tile->Temperature = 0;
			_Tile->Unbuildable = 0;
			_Tile->TileSheet = 0;
			_Tile->TileVar = 0;
		}
	}
}

void ScreenToTile(const SDL_Point* _Screen, struct SDL_Point* _Hex) {
	SDL_Point _SectPxl = {0};
	uint32_t _SectX = _Screen->x / (2 * TILE_DIST);
	uint32_t _SectY = _Screen->y / (TILE_HEIGHT_THIRD);
	uint32_t _ArrayX = _SectX;
	uint32_t _ArrayY = _SectY;

	_SectPxl.x = _Screen->x % (2 * TILE_DIST);
	_SectPxl.y = _Screen->y % (TILE_HEIGHT_THIRD);
	if((_SectY & 1) == 0) {
		//A
		if(_SectPxl.y < ((TILE_HEIGHT / 4) - _SectPxl.x * TILE_GRADIENT)) {
			_ArrayX = _SectX - 1;
			_ArrayY = _SectY - 1;
		} else if(_SectPxl.y < (-(TILE_HEIGHT / 4) + _SectPxl.x * TILE_GRADIENT)) {
			_ArrayY = _SectY - 1;
		}
	} else {
		//B
		if(_SectPxl.x >= (TILE_DIST)) {
			if(_SectPxl.y < (2 * (TILE_HEIGHT / 4) - _SectPxl.x * TILE_GRADIENT)) {
			//	_ArrayX = _SectX - 1;
				_ArrayY = _SectY - 1;
			}
		} else {
			if(_SectPxl.y < (_SectPxl.x * TILE_GRADIENT)) {
				_ArrayY = _SectY - 1;
			} else {
				_ArrayX = _SectX - 1;
			}
		}
	}
	_Hex->x = _ArrayX;
	_Hex->y = _ArrayY;
	/*if(_Hex->x < 0 || _Hex->x > _Map->TileLength
			|| _Hex->y < 0 || _Hex->y > _Map->TileLength)
		return;*/
}

/*void HexRound(float x, float y, float z, SDL_Point* _Out) {
	float rx = round(x);
	float ry = round(y);
	float rz = round(z);
	float xDiff = abs(rx - x);
	float yDiff = abs(ry - y);
	float zDiff = abs(rz - z); 

	if(xDiff > yDiff && xDiff > zDiff) { 
		rz = -ry - rz;	
	} else if(yDiff > zDiff) {
		ry = -rx - rz;
	} else {
		rz = -rz - ry;
	}
	_Out->x = rx;
	_Out->y = rz;
}

void ScreenToTile(const SDL_Point* _Screen, SDL_Point* _Hex) {
	float q = (_Screen->x * sqrt3 / 3 - _Screen->y / 3) / (TILE_WIDTH / 2);
	float r  = _Screen->y * 2/3 / (TILE_WIDTH / 2); 

	HexRound(q, -q-r, r, _Hex);
	_Hex->y = _Hex->y + (_Hex->x - (_Hex->x & 1)) / 2;
}*/

void TileRing(struct MapRenderer* _Renderer, SDL_Point* _Center, uint32_t _Radius, struct Tile** _Out) {
	int _Count = 0;
	SDL_Point _Tile = {_Center->x, _Center->y};
	struct Tile* _Temp = NULL;

	for(int i = 0; i < 6; ++i) {
		for(int j = 0; j < _Radius; ++j) {
			TileNeighbor(i, &_Tile, &_Tile);
			_Temp = MapGetTile(_Renderer, &_Tile);
			if(_Temp != NULL)
				_Out[_Count++] = _Temp;
		}
	}
}

void TilesInRange(struct MapRenderer* _Renderer, const SDL_Point* _Pos, int _Range, struct LinkedList* _List) {
	SDL_Point _Point = {_Pos->x - _Range, max(-_Range, -_Pos->x - _Range)};
	const struct Tile* _Tile = NULL;

	for(; _Point.x <= _Range; ++_Point.x)
		for(; _Point.y <= min(_Range, -_Pos->x - _Range); ++_Point.y)
			if((_Tile = MapGetTile(_Renderer, &_Point)) != NULL)
				LnkLstPushBack(_List, (struct Tile*) _Tile);
}

void MapTileRenderRect(const struct MapRenderer* _Renderer, const SDL_Point* _TilePos, SDL_Rect* _Rect) {
	_Rect->x = (_TilePos->x * TILE_WIDTH) - (_Renderer->Screen.x * TILE_WIDTH);
	_Rect->y = (_TilePos->y * TILE_HEIGHT_THIRD) - (_Renderer->Screen.y * TILE_HEIGHT_THIRD);
	if((_TilePos->y & 1) == 1) {
		_Rect->x += (TILE_WIDTH / 2);
	}
	_Rect->w = TILE_WIDTH;
	_Rect->h = TILE_HEIGHT;
}

void MapRender(SDL_Renderer* _Renderer, struct MapRenderer* _Map) {
	struct LinkedList _QuadList = {0, NULL, NULL};
	struct LnkLst_Node* _Itr = NULL;
	struct Tile* _Tile = NULL;
	struct Sprite* _Sprite = NULL;

/*	MapObjectsInRect(_Map, MAPRENDER_TILE, &_Map->Screen, &_QuadList);
	_Itr = _QuadList.Front;
	while(_Itr != NULL) {*/
	for(int x = 0; x < _Map->Screen.w; ++x) {
		for(int y = 0; y < _Map->Screen.h; ++y) {
			_Tile = &_Map->Tiles[(_Map->Screen.y + y) * _Map->TileLength + (_Map->Screen.x + x)];

			const struct TileSheet* _TileSheet = &_Map->TileSheets[_Tile->TileSheet]; 
			SDL_Rect _Rect = {_TileSheet->VarPos[_Tile->TileVar].x, _TileSheet->VarPos[_Tile->TileVar].x, TILE_WIDTH, TILE_HEIGHT};
			SDL_Rect _SpritePos = {x * TILE_WIDTH, y * TILE_HEIGHT_THIRD, TILE_WIDTH, TILE_HEIGHT};
			if((y & 1) == 1) {
				_SpritePos.x += (TILE_WIDTH / 2);
			}

			SDL_RenderCopy(g_Renderer, _TileSheet->Tiles, &_Rect, &_SpritePos);
		}
	}
//	}
	LnkLstClear(&_QuadList);
	for(int _Layer = MAPRENDER_TILE + 1; _Layer < MAPRENDER_LAYERS; ++_Layer) {
		MapObjectsInRect(_Map, _Layer, &_Map->Screen, &_QuadList);
		_Itr = _QuadList.Front;
		while(_Itr != NULL) {
			_Sprite = (struct Sprite*)_Itr->Data;
			SpriteOnDraw(_Sprite);
			_Itr = _Itr->Next;
		}
		LnkLstClear(&_QuadList);
	}
}

void MapObjectsInRect(struct MapRenderer* _Renderer, int _Layer, const SDL_Rect* _Rect, struct LinkedList* _Data) {
	if(_Layer < 0 || _Layer >= MAPRENDER_LAYERS)
		return;
	QTPointInRectangle(&_Renderer->RenderArea[_Layer], _Rect, (void(*)(const void*, SDL_Point*))SpriteGetTilePos, _Data);
}

const struct Tile* MapGetTileConst(const struct MapRenderer* const _Renderer, const SDL_Point* _Point) {
	return MapGetTile((struct MapRenderer*)_Renderer, _Point);
}

struct Tile* MapGetTile(struct MapRenderer* _Renderer, const SDL_Point* _Point) {
	if(_Point->x < 0 || _Point->x >= _Renderer->TileLength || _Point->y < 0 || _Point->y >= _Renderer->TileLength)
		return NULL;
	return &_Renderer->Tiles[(_Point->y * _Renderer->TileLength) + _Point->x];
}

void MapDrawColorOverlay(const struct MapRenderer* _Renderer, const SDL_Point* _Point, SDL_Color* _Color) {
	SDL_Rect _Rect;

	SDL_SetRenderDrawColor(g_Renderer, _Color->r, _Color->g, _Color->b, _Color->a);
	MapTileRenderRect(_Renderer, _Point, &_Rect);
	SDL_RenderFillRect(g_Renderer, &_Rect);
}

void MapGetUnitPos(const void* _Data, SDL_Point* _Pos) {
	_Pos->x = ((struct Sprite*)_Data)->TilePos.x;
	_Pos->y = ((struct Sprite*)_Data)->TilePos.y;
}

void MapGetSettlementPos(const void* _Data, SDL_Point* _Point) {
	const struct Settlement* _Loc = (struct Settlement*) _Data;

	_Point->x = _Loc->Pos.x;
	_Point->y = _Loc->Pos.y;
}

struct Army* MapGetUnit(struct MapRenderer* _Renderer, const SDL_Point* _Point) {
	if(_Point->x < 0 || _Point->x >= _Renderer->TileLength || _Point->y < 0 || _Point->y >= _Renderer->TileLength)
		return NULL;
	return (struct Army*) QTGetPoint(&_Renderer->RenderArea[MAPRENDER_UNIT], _Point, MapGetUnitPos);
}

int MapUnitCanMove(struct MapRenderer* _Renderer, struct Army* _Army, const SDL_Point* _Point) {
	return ((!((int) (QTGetPoint(&_Renderer->RenderArea[MAPRENDER_UNIT], _Point, MapGetUnitPos)))) && _Army->InBattle == 0);
}

int MapMoveUnit(struct MapRenderer* _Renderer, struct Army* _Army, const SDL_Point* _Point) {
	if(MapUnitCanMove(_Renderer, _Army, _Point)) {
		QTRemovePoint(&_Renderer->RenderArea[MAPRENDER_UNIT], &_Army->Sprite.TilePos, MapGetUnitPos);
		QTInsertPoint(&_Renderer->RenderArea[MAPRENDER_UNIT], &_Army->Sprite, _Point);
		SpriteSetTilePos(&_Army->Sprite, g_GameWorld.MapRenderer, _Point);
		return 1;
	}
	return 0;
}
