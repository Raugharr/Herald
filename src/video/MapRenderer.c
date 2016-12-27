/*
 * File: MapRenderer.c
 * Author: David Brotz
 */

#include "MapRenderer.h"

#include "Sprite.h"
#include "Tile.h"
#include "Video.h"
#include "MapGenerator.h"

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

void CreateBumpMap(struct MapRenderer* Map, uint16_t MapLength, float* HeightMap) {
	int Radius = 6;
	int TileSize = NumTileRadius(Radius);
	struct Tile* Tiles[TileSize];
	SDL_Point Center = {10, 10};
	double val = cos(0.314);

	memset(Tiles, 0, sizeof(Tiles));
//	WhiteNoiseGen(MapLength, MapLength, HeightMap);
	//CreateHeightMap(MapLength, MapLength, HeightMap, 0.05f, 0.01f, 100);

	HeightMapTexture("HeightMap.bmp", MapLength, MapLength, HeightMap);
//	BrownNoise(MapLength, MapLength, HeightMap);
	HeightMapTexture("HeightMapBrown.bmp", MapLength, MapLength, HeightMap);
	TileSpiral(Map, &Center, Radius, Tiles);
	HeightMap[TileToIndex(Map, Tiles[0])] = 0.99f;
	for(int i = 1; i < TileSize; ++i) {
		HeightMap[TileToIndex(Map, Tiles[i])] = 0.8f;
	}

	//CreateHeightMap(MapLength, MapLength, RainFall, 0.2f, 0.05f, 40);
	//HeightMapTexture("RainFall.bmp", MapLength, MapLength, RainFall);
}

struct MapRenderer* CreateMapRenderer(int MapLength, SDL_Point* RenderSize) {
	struct MapRenderer* Map = (struct MapRenderer*) malloc(sizeof(struct MapRenderer));
	struct Tile* Tile = NULL;
	float* HeightMap = calloc(MapLength * MapLength, sizeof(float));
	//float* RainFall = calloc(MapLength * MapLength, sizeof(float));

	Map->Selector = ResourceGet("Select.png");
	Map->TileSheets[0].TileFile = ResourceGet("grass.png");
	Map->TileSheets[0].Tiles = ResourceGetData(Map->TileSheets[0].TileFile);
	Map->TileSheets[0].VarPos[0].x = 0;
	Map->TileSheets[0].VarPos[0].y = 0;

	Map->TileSheets[1].TileFile = ResourceGet("hill.png");
	Map->TileSheets[1].Tiles = ResourceGetData(Map->TileSheets[1].TileFile);
	Map->TileSheets[1].VarPos[0].x = 0;
	Map->TileSheets[1].VarPos[0].y = 0;

	Map->TileSheets[2].TileFile = ResourceGet("mountain.png");
	Map->TileSheets[2].Tiles = ResourceGetData(Map->TileSheets[2].TileFile);
	Map->TileSheets[2].VarPos[0].x = 0;
	Map->TileSheets[2].VarPos[0].y = 0;

	Map->TileSheets[3].TileFile = ResourceGet("bog.png");
	Map->TileSheets[3].Tiles = ResourceGetData(Map->TileSheets[3].TileFile);
	Map->TileSheets[3].VarPos[0].x = 0;
	Map->TileSheets[3].VarPos[0].y = 0;

	Map->TileArea = MapLength * MapLength;
	Map->TileLength = MapLength;
	Map->Tiles = calloc(Map->TileArea, sizeof(struct Tile));
	Map->IsRendering = 0;
	for(int i = 0; i < MAPRENDER_LAYERS; ++i) {
		Map->RenderArea[i].NorthEast = NULL;
		Map->RenderArea[i].NorthWest = NULL;
		Map->RenderArea[i].SouthWest = NULL;
		Map->RenderArea[i].SouthEast = NULL;
		Map->RenderArea[i].Data = NULL;
		Map->RenderArea[i].BoundingBox.x = 0;
		Map->RenderArea[i].BoundingBox.y = 0;
		Map->RenderArea[i].BoundingBox.w = Map->TileLength * TILE_WIDTH;
		Map->RenderArea[i].BoundingBox.h = Map->TileLength * TILE_HEIGHT;
	}
	Map->Screen.x = 0;
	Map->Screen.y = 0;
	Map->Screen.w = RenderSize->x;
	Map->Screen.h = RenderSize->y;
	CreateBumpMap(Map, MapLength, HeightMap);
	for(int x = 0; x < Map->TileLength; ++x) {
		for(int y = 0; y < Map->TileLength; ++y) {
			Tile = &Map->Tiles[y * Map->TileLength + x];
			Tile->Temperature = 0;
			if(HeightMap[y * MapLength + x] >= 0.85f)
				Tile->TileSheet = 2;
			else if(HeightMap[y * MapLength + x] >= 0.65)
				Tile->TileSheet = 1;
			else
				Tile->TileSheet = 0;
			Tile->TileVar = 0;
		}
	}
	/*for(int x = 0; x < Map->TileLength; ++x) {
		for(int y = 0; y < Map->TileLength; ++y) {
			Tile = &Map->Tiles[y * Map->TileLength + x];
			if(HeightMap[y * MapLength + x] < 0.25 && RainFall[y * MapLength + x] > 0.50f)
				Tile->TileSheet = 3;
		}
	}*/
	
	//MapLoad(Map);
	return Map;
}

void DestroyMapRenderer(struct MapRenderer* Map) {
	SDL_DestroyTexture(Map->Settlement);
	SDL_DestroyTexture(Map->Warrior);
}

void MapLoad(struct MapRenderer* Map) {
	struct Tile* Tile = NULL;

	for(int y = 0; y < Map->TileLength; ++y) {
		for(int x = 0; x < Map->TileLength; ++x) {
			Tile = &Map->Tiles[y * Map->TileLength + x];
			Tile->Temperature = 0;
			Tile->TileSheet = 0;
			Tile->TileVar = 0;
		}
	}
}

void ScreenToHex(const SDL_Point* Screen, struct SDL_Point* Hex) {
	SDL_Point SectPxl = {0};
	uint32_t SectX = Screen->x / (2 * TILE_DIST);
	uint32_t SectY = Screen->y / (TILE_HEIGHT_THIRD);
	uint32_t ArrayX = SectX;
	uint32_t ArrayY = SectY;

	SectPxl.x = Screen->x % (2 * TILE_DIST);
	SectPxl.y = Screen->y % (TILE_HEIGHT_THIRD);
	if((SectY & 1) == 0) {
		//A
		if(SectPxl.y < ((TILE_HEIGHT / 4) - SectPxl.x * TILE_GRADIENT)) {
			ArrayX = SectX - 1;
			ArrayY = SectY - 1;
		} else if(SectPxl.y < (-(TILE_HEIGHT / 4) + SectPxl.x * TILE_GRADIENT)) {
			ArrayY = SectY - 1;
		}
	} else {
		//B
		if(SectPxl.x >= (TILE_DIST)) {
			if(SectPxl.y < (2 * (TILE_HEIGHT / 4) - SectPxl.x * TILE_GRADIENT)) {
			//	ArrayX = SectX - 1;
				ArrayY = SectY - 1;
			}
		} else {
			if(SectPxl.y < (SectPxl.x * TILE_GRADIENT)) {
				ArrayY = SectY - 1;
			} else {
				ArrayX = SectX - 1;
			}
		}
	}
	Hex->x = ArrayX;
	Hex->y = ArrayY;
	/*if(Hex->x < 0 || Hex->x > Map->TileLength
			|| Hex->y < 0 || Hex->y > Map->TileLength)
		return;*/
}

/*void HexRound(float x, float y, float z, SDL_Point* Out) {
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
	Out->x = rx;
	Out->y = rz;
}

void ScreenToTile(const SDL_Point* Screen, SDL_Point* Hex) {
	float q = (Screen->x * sqrt3 / 3 - Screen->y / 3) / (TILE_WIDTH / 2);
	float r  = Screen->y * 2/3 / (TILE_WIDTH / 2); 

	HexRound(q, -q-r, r, Hex);
	Hex->y = Hex->y + (Hex->x - (Hex->x & 1)) / 2;
}*/

void TileRing(struct MapRenderer* Renderer, const SDL_Point* Center, uint16_t Radius, struct Tile** Out) {
	SDL_Point Tile = {Center->x, Center->y};
	struct Tile* Temp = NULL;
	uint32_t Count = 0;

	for(int i = 0; i < TILE_SIZE; ++i) {
		for(int j = 1; j < Radius; ++j) {
			TileNeighbor(i, &Tile, &Tile);
			Temp = MapGetTile(Renderer, &Tile);
			if(Temp != NULL)
				Out[Count++] = Temp;
		}
	}
}

void TileSpiral(struct MapRenderer* Renderer, const SDL_Point* Center, uint16_t Radius, struct Tile** Out) {
	uint32_t Count = 0;
	SDL_Point Tile = {Center->x, Center->y};

	Out[Count++] = MapGetTile(Renderer, Center);
	for(int i = 2; i <= Radius; ++i, Count += (i - 2) * TILE_SIZE) {
		TileNeighbor(TILE_SOUTHWEST, &Tile, &Tile);
		TileRing(Renderer, &Tile, i, &Out[Count]);
	}
}

void TilesInRange(struct MapRenderer* Renderer, const SDL_Point* Pos, int Range, struct LinkedList* List) {
	SDL_Point Point = {Pos->x - Range, max(-Range, -Pos->x - Range)};
	const struct Tile* Tile = NULL;

	for(; Point.x <= Range; ++Point.x)
		for(; Point.y <= min(Range, -Pos->x - Range); ++Point.y)
			if((Tile = MapGetTile(Renderer, &Point)) != NULL)
				LnkLstPushBack(List, (struct Tile*) Tile);
}

void MapTileRenderRect(const struct MapRenderer* Renderer, const SDL_Point* TilePos, SDL_Rect* Rect) {
	Rect->x = (TilePos->x * TILE_WIDTH) - (Renderer->Screen.x * TILE_WIDTH);
	Rect->y = (TilePos->y * TILE_HEIGHT_THIRD) - (Renderer->Screen.y * TILE_HEIGHT_THIRD);
	if((TilePos->y & 1) == 1) {
		Rect->x += (TILE_WIDTH / 2);
	}
	Rect->w = TILE_WIDTH;
	Rect->h = TILE_HEIGHT;
}

void MapRender(SDL_Renderer* Renderer, struct MapRenderer* Map) {
	struct LinkedList QuadList = {0, NULL, NULL};
	struct LnkLst_Node* Itr = NULL;
	struct Tile* Tile = NULL;
	struct Sprite* Sprite = NULL;

/*	MapObjectsInRect(Map, MAPRENDER_TILE, &Map->Screen, &QuadList);
	Itr = QuadList.Front;
	while(Itr != NULL) {*/
	for(int x = 0; x < Map->Screen.w; ++x) {
		for(int y = 0; y < Map->Screen.h; ++y) {
			Tile = &Map->Tiles[(Map->Screen.y + y) * Map->TileLength + (Map->Screen.x + x)];

			const struct TileSheet* TileSheet = &Map->TileSheets[Tile->TileSheet]; 
			SDL_Rect Rect = {TileSheet->VarPos[Tile->TileVar].x, TileSheet->VarPos[Tile->TileVar].x, TILE_WIDTH, TILE_HEIGHT};
			SDL_Rect SpritePos = {x * TILE_WIDTH, y * TILE_HEIGHT_THIRD, TILE_WIDTH, TILE_HEIGHT};
			if((Map->Screen.y & 1) == 0) {
				if((y & 1) == 1) {
					SpritePos.x += (TILE_WIDTH / 2);
				}
			} else {
				if((y & 1) == 0) {
					SpritePos.x += (TILE_WIDTH / 2);
				}
			}

			if(SDL_RenderCopy(g_Renderer, TileSheet->Tiles, &Rect, &SpritePos) < 0) {
				Log(ELOG_ERROR, "%s", SDL_GetError());
			}
		}
	}
//	}
	LnkLstClear(&QuadList);
	for(int Layer = MAPRENDER_TILE + 1; Layer < MAPRENDER_LAYERS; ++Layer) {
		MapObjectsInRect(Map, Layer, &Map->Screen, &QuadList);
		Itr = QuadList.Front;
		while(Itr != NULL) {
			Sprite = (struct Sprite*)Itr->Data;
			SpriteOnDraw(Sprite);
			Itr = Itr->Next;
		}
		LnkLstClear(&QuadList);
	}
}

void MapObjectsInRect(struct MapRenderer* Renderer, int Layer, const SDL_Rect* Rect, struct LinkedList* Data) {
	if(Layer < 0 || Layer >= MAPRENDER_LAYERS)
		return;
	QTPointInRectangle(&Renderer->RenderArea[Layer], Rect, (void(*)(const void*, SDL_Point*))SpriteGetTilePos, Data);
}

const struct Tile* MapGetTileConst(const struct MapRenderer* const Renderer, const SDL_Point* Point) {
	return MapGetTile((struct MapRenderer*)Renderer, Point);
}

struct Tile* MapGetTile(struct MapRenderer* Renderer, const SDL_Point* Point) {
	if(Point->x < 0 || Point->x >= Renderer->TileLength || Point->y < 0 || Point->y >= Renderer->TileLength)
		return NULL;
	return &Renderer->Tiles[(Point->y * Renderer->TileLength) + Point->x];
}

void MapDrawColorOverlay(const struct MapRenderer* Renderer, const SDL_Point* Point, SDL_Color* Color) {
	SDL_Rect Rect;

	SDL_SetRenderDrawColor(g_Renderer, Color->r, Color->g, Color->b, Color->a);
	MapTileRenderRect(Renderer, Point, &Rect);
	SDL_RenderFillRect(g_Renderer, &Rect);
}

void MapGetUnitPos(const void* Data, SDL_Point* Pos) {
	Pos->x = ((struct Sprite*)Data)->TilePos.x;
	Pos->y = ((struct Sprite*)Data)->TilePos.y;
}

void MapGetSettlementPos(const void* Data, SDL_Point* Point) {
	const struct Settlement* Loc = (struct Settlement*) Data;

	Point->x = Loc->Pos.x;
	Point->y = Loc->Pos.y;
}

struct Army* MapGetUnit(struct MapRenderer* Renderer, const SDL_Point* Point) {
	if(Point->x < 0 || Point->x >= Renderer->TileLength || Point->y < 0 || Point->y >= Renderer->TileLength)
		return NULL;
	return (struct Army*) QTGetPoint(&Renderer->RenderArea[MAPRENDER_UNIT], Point, MapGetUnitPos);
}

int MapUnitCanMove(struct MapRenderer* Renderer, struct Army* Army, const SDL_Point* Point) {
	return ((!((int) (QTGetPoint(&Renderer->RenderArea[MAPRENDER_UNIT], Point, MapGetUnitPos)))) && Army->InBattle == 0);
}

int MapMoveUnit(struct MapRenderer* Renderer, struct Army* Army, const SDL_Point* Point) {
	if(MapUnitCanMove(Renderer, Army, Point)) {
		QTRemovePoint(&Renderer->RenderArea[MAPRENDER_UNIT], &Army->Sprite.TilePos, MapGetUnitPos);
		QTInsertPoint(&Renderer->RenderArea[MAPRENDER_UNIT], &Army->Sprite, Point);
		SpriteSetTilePos(&Army->Sprite, g_GameWorld.MapRenderer, Point);
		return 1;
	}
	return 0;
}
