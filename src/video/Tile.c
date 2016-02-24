/*
 * File: Tile.c
 * Author: David Brotz
 */

#include "Tile.h"

#include "MapRenderer.h"
#include "QuadTree.h"
#include "Sprite.h"

#ifdef DEBUG
#include "../sys/Log.h"
#endif

#include <SDL2/SDL.h>
#include <stdlib.h>

static const SDL_Point g_TileEvenOffsets[] = {
		{-1, -1},
		{0, -1},
		{1, 0},
		{0, 1},
		{-1, 1},
		{-1, 0}
};

static const SDL_Point g_TileOddOffsets[] = {
		{0, -1},
		{1, -1},
		{1, 0},
		{1, 1},
		{0, 1},
		{-1, 0}
};

struct Tile* CreateTile(struct MapRenderer* _Renderer, struct Resource* _Image, int _X, int _Y) {
	struct Tile* _Tile = (struct Tile*) malloc(sizeof(struct Tile));

	_Tile->Image = _Image;
	_Tile->Rect.x = 0;
	_Tile->Rect.y = 0;
	_Tile->Rect.w = TILE_WIDTH;
	_Tile->Rect.h = TILE_HEIGHT;
	_Tile->TilePos.x = _X;
	_Tile->TilePos.y = _Y;
	_Tile->SpritePos.x = _X * TILE_WIDTH;
	_Tile->SpritePos.y = _Y * TILE_HEIGHT_THIRD;
	if((_Y & 1) == 1) {
		_Tile->SpritePos.x += (TILE_WIDTH / 2);
	}
	_Tile->SpritePos.w = TILE_WIDTH;
	_Tile->SpritePos.h = TILE_HEIGHT;
	_Tile->Forest = 0;
	_Tile->Temperature = 0;
	_Tile->Unbuildable = 0;
	QTInsertPoint(&_Renderer->RenderArea[MAPRENDER_TILE], _Tile, &_Tile->TilePos);
	return _Tile;
}

void DestroyTile(struct Tile* _Tile) {
	free(_Tile);
}

void GetAdjPos(const SDL_Point* _Pos, SDL_Point* _Adj, int _TileDir) {
	const SDL_Point* _TileAdj = NULL;

	#ifdef DEBUG
		if(_TileDir < 0 || _TileDir > TILE_WEST) {
			Log(ELOG_ERROR, "GetAdjTile passed an invalid tile direction %i.", _TileDir);
			return;
		}
	#endif
	if(_Pos == NULL || _Adj == NULL)
		return;
	_TileAdj = ((_Pos->y & 1) == 1) ? (&g_TileOddOffsets[_TileDir]) : (&g_TileEvenOffsets[_TileDir]);
	_Adj->x = _Pos->x + _TileAdj->x;
	_Adj->y = _Pos->y +_TileAdj->y;
}

struct Tile* GetAdjTile(struct MapRenderer* _Map, const struct Tile* _Tile, int _TileDir) {
	const SDL_Point* _TileAdj = NULL;
	SDL_Point _Pos;

#ifdef DEBUG
	if(_TileDir < 0 || _TileDir > TILE_WEST) {
		Log(ELOG_ERROR, "GetAdjTile passed an invalid tile direction %i.", _TileDir);
		return NULL;
	}
#endif
	if(_Tile == NULL)
		return NULL;
	_Pos.x = _Tile->TilePos.x;
	_Pos.y = _Tile->TilePos.y;
	_TileAdj = ((_Tile->TilePos.y & 1) == 1) ? (&g_TileOddOffsets[_TileDir]) : (&g_TileEvenOffsets[_TileDir]);
	_Pos.x += _TileAdj->x;
	_Pos.y += _TileAdj->y;
	return MapGetTile(_Map, &_Pos);
}

void TileGetAdjTiles(struct MapRenderer* _Renderer, const struct Tile* _Tile, struct Tile** _AdjTiles) {
	SDL_Point _Offset;

	if((_Tile->TilePos.y & 1) == 0) {
		for(int i = 0; i < TILE_SIZE; ++i) {
				_Offset.x = _Tile->TilePos.x + g_TileEvenOffsets[i].x;
				_Offset.y = _Tile->TilePos.y + g_TileEvenOffsets[i].y;
				_AdjTiles[i] = MapGetTile(_Renderer, &_Offset);
			}
	} else {
		for(int i = 0; i < TILE_SIZE; ++i) {
			_Offset.x = _Tile->TilePos.x + g_TileOddOffsets[i].x;
			_Offset.y = _Tile->TilePos.y + g_TileOddOffsets[i].y;
			_AdjTiles[i] = MapGetTile(_Renderer, &_Offset);
		}
	}
}

void TileAdjTileOffset(const struct SDL_Point* _Tile, int _Direction, SDL_Point* _Pos) {
	if(_Direction >= TILE_SIZE) {
		_Pos->x = _Tile->x;
		_Pos->y = _Tile->y;
		return;
	}
	if((_Tile->y & 1) == 0) {
		_Pos->x = _Tile->x + g_TileEvenOffsets[_Direction].x;
		_Pos->y = _Tile->y + g_TileEvenOffsets[_Direction].y;
	} else {
		_Pos->x = _Tile->x + g_TileOddOffsets[_Direction].x;
		_Pos->y = _Tile->y + g_TileOddOffsets[_Direction].y;
	}
}

int TileGetDistance(const struct SDL_Point* _Start, const struct SDL_Point* _End) {
	//return sign(1);
	return (abs(_Start->x - _End->x) + abs(_Start->y - _End->y) + abs((-_Start->x - _Start->y) - (-_End->x - _End->y))) / 2;
}

int TileNextInRing(const SDL_Point* _Point, SDL_Point* _Adj, int _RingUsed, int _Radius) {
	int _Dir = TILE_EAST - 1;
	int _Ct = 0;

	if((_RingUsed % TILE_SIZE) == 0) {
		if(_RingUsed == 0)
			--_Radius;
		TileNextRing(_Point, _Adj, _Radius);
		return TILE_EAST;
	} else {
		while(_RingUsed > 0) {
			++_Ct;
			--_RingUsed;
			if(_Ct >= _Radius) {
				++_Dir;
				_Ct = 0;
			}
		}
	}
	if(_Dir == TILE_SIZE)
		_Dir = TILE_NORTHWEST;
	GetAdjPos(_Point, _Adj, _Dir);
	return _Dir;
}

int TileNextRing(const SDL_Point* _Point, SDL_Point* _New, int _Radius) {
	_New->x = _Point->x;
	if(_Radius == 0) {
		GetAdjPos(_Point, _New, TILE_NORTHWEST);
		return TILE_NORTHWEST;
	}
	_New->y = _Point->y - 2;
	return TILE_SIZE;
}
