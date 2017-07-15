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

	size_t TileSize = TILE_SIZE;//NumTileRadius(3); 
	struct Tile* TileList[TileSize];
	SDL_Point TilePos = {4, 6};

	//memset(TileList, 0, sizeof(TileList));
	TileRing(Map, &TilePos, 2, TileList); 
	CreatePerlinNoise(MapLength, MapLength, HeightMap);
	HeightMapTexture("HeightMap.bmp", MapLength, MapLength, HeightMap);
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

	Map->TileSheets[4].TileFile = ResourceGet("water.png");
	Map->TileSheets[4].Tiles = ResourceGetData(Map->TileSheets[4].TileFile);
	Map->TileSheets[4].VarPos[0].x = 0;
	Map->TileSheets[4].VarPos[0].y = 0;

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
			if(HeightMap[y * MapLength + x] >= 0.50f) {
				Tile->TileSheet = 2;
				HeightMap[y * MapLength + x] = 0.75f;
			}
			else if(HeightMap[y * MapLength + x] >= 0.30f) {
				Tile->TileSheet = 1;
				HeightMap[y * MapLength + x] = 0.65f;
			}
			else if(HeightMap[y * MapLength + x] <= -0.60f) {
				Tile->TileSheet = 4;
				HeightMap[y * MapLength + x] = 0.20f;
			}
			else {
				Tile->TileSheet = 0;
				HeightMap[y * MapLength + x] = 0.40f;
			}
			Tile->TileVar = 0;
		}
	}
	HeightMapTexture("HeightMapFinal.bmp", MapLength, MapLength, HeightMap);
	/*for(int x = 0; x < Map->TileLength; ++x) {
		for(int y = 0; y < Map->TileLength; ++y) {
			Tile = &Map->Tiles[y * Map->TileLength + x];
			if(HeightMap[y * MapLength + x] < 0.25 && RainFall[y * MapLength + x] > 0.50f)
				Tile->TileSheet = 3;
		}
	}*/
	
	//MapLoad(Map);
	free(HeightMap);
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
	static uint8_t Neighbors[TILE_SIZE] = {TILE_NORTHEAST, TILE_EAST, TILE_SOUTHEAST, TILE_SOUTHWEST, TILE_WEST, TILE_NORTHWEST};

	TileNeighbor(TILE_WEST, &Tile, &Tile);
	for(int i = 0; i < TILE_SIZE; ++i) {
		for(int j = 1; j < Radius; ++j) {
			TileNeighbor(Neighbors[i], &Tile, &Tile);
			Temp = MapGetTile(Renderer, &Tile);
			Out[Count++] = Temp;
		}
	}
}

void TileSpiral(struct MapRenderer* Renderer, const SDL_Point* Center, uint16_t Radius, struct Tile** Out) {
	uint32_t Count = 0;
	SDL_Point Tile = {Center->x, Center->y};

	Out[Count++] = MapGetTile(Renderer, Center);
	for(int i = 2; i <= Radius; ++i, Count += (i - 2) * TILE_SIZE) {
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

void MapRenderAll(SDL_Renderer* Renderer, struct MapRenderer* Map) {
	struct LinkedList QuadList = {0, NULL, NULL};
	struct Tile* Tile = NULL;

/*	MapObjectsInRect(Map, MAPRENDER_TILE, &Map->Screen, &QuadList);
	Itr = QuadList.Front;
	while(Itr != NULL) {*/
	for(int x = 0; x < Map->Screen.w; ++x) {
		for(int y = 0; y < Map->Screen.h; ++y) {
			Tile = &Map->Tiles[(Map->Screen.y + y) * Map->TileLength + (Map->Screen.x + x)];

			const struct TileSheet* TileSheet = &Map->TileSheets[Tile->TileSheet]; 
			SDL_Rect Rect = {TileSheet->VarPos[Tile->TileVar].x, TileSheet->VarPos[Tile->TileVar].x, TILE_WIDTH, TILE_HEIGHT};
			SDL_Rect SpritePos = {x * TILE_WIDTH, y * TILE_HEIGHT_THIRD, TILE_WIDTH, TILE_HEIGHT};

			MapHexOffset(Map->Screen.x, Map->Screen.y, y, (uint16_t*)&SpritePos.x);
			if(SDL_RenderCopy(g_Renderer, TileSheet->Tiles, &Rect, &SpritePos) < 0) {
				Log(ELOG_ERROR, "%s", SDL_GetError());
			}
		}
	}
//	}
	LnkLstClear(&QuadList);
	for(int Layer = MAPRENDER_TILE + 1; Layer < MAPRENDER_LAYERS; ++Layer) {
		uint32_t TableSz = FrameSizeRemain() / sizeof(struct Sprite*);
		struct Sprite** SpriteList = SAlloc(TableSz * sizeof(struct Sprite*));
		uint32_t ListSize = 0;

		MapObjectsInRect(Map, Layer, &Map->Screen, (void**)SpriteList, &ListSize, TableSz);
		for(int i = 0; i < ListSize; ++i)
			SpriteOnDraw(Renderer, SpriteList[i], Map->Screen.x * TILE_WIDTH, Map->Screen.y * TILE_HEIGHT_THIRD);
		SFree(sizeof(void*) * TableSz);
	}
}

void MapObjectsInRect(struct MapRenderer* Renderer, uint8_t Layer, const SDL_Rect* Rect, void** Data, uint32_t* Size, uint32_t TableSz) {
	if(Layer < 0 || Layer >= MAPRENDER_LAYERS)
		return;
	QTPointInRectangle(&Renderer->RenderArea[Layer], Rect, (void(*)(const void*, SDL_Point*))SpriteGetTilePos, Data, Size, TableSz);
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

void MapGetSettlementPos(const void* Data, SDL_Point* Point) {
	const struct Settlement* Loc = (struct Settlement*) Data;

	Point->x = Loc->Pos.x;
	Point->y = Loc->Pos.y;
}

struct Army* MapGetUnit(struct MapRenderer* Renderer, const SDL_Point* Point) {
	if(Point->x < 0 || Point->x >= Renderer->TileLength || Point->y < 0 || Point->y >= Renderer->TileLength)
		return NULL;
	return (struct Army*) QTGetPoint(&Renderer->RenderArea[MAPRENDER_UNIT], Point, (void(*)(const void*, SDL_Point*))SpriteGetTilePos);
}

int MapUnitCanMove(struct MapRenderer* Renderer, struct Army* Army, const SDL_Point* Point) {
	return ((!((int) (QTGetPoint(&Renderer->RenderArea[MAPRENDER_UNIT], Point, (void(*)(const void*, SDL_Point*))SpriteGetTilePos)))) && Army->InBattle == 0);
}

int MapMoveUnit(struct MapRenderer* Renderer, struct Army* Army, const SDL_Point* Point) {
	if(MapUnitCanMove(Renderer, Army, Point)) {
		//Causes crash.
		QTRemoveNode(&Renderer->RenderArea[MAPRENDER_UNIT], &Army->Sprite.TilePos, (void(*)(const void*, SDL_Point*))SpriteGetTilePos, &Army->Sprite);
		QTInsertPoint(&Renderer->RenderArea[MAPRENDER_UNIT], &Army->Sprite, Point);
		SpriteSetTilePos(&Army->Sprite, g_GameWorld.MapRenderer, Point);
		return 1;
	}
	return 0;
}

void ShapeMountains(uint16_t Width, uint16_t Length, uint8_t* Map) {
	for(int x = 0; x < Width; ++x) {
		for(int y = 0; y < Length; ++y) {
			uint8_t* Pixel = &Map[y * Width + x];
			uint32_t Min = 0;

			if(*Pixel == 1) {
				*Pixel = 0;
			} else {
				*Pixel = 0xFF;
				if(x > 0) Min = min(*Pixel, *(Pixel - 1) + 1); //*(Pixel - 1) is Map[x - 1][y].
				if(y > 0) Min = min(*Pixel, *(Pixel - Width) + 1); //*(Pixel - Width) is Map[x][y - 1].
				if(Min > 0xFF) Min = 0xFF;
				*Pixel = Min;
			}
		}
	}
	for(int x = Width - 1; x >= 0; --x) {
		for(int y = Length - 1; y >= 0; --y) {
			uint8_t* Pixel = &Map[y * Width + x];
			uint32_t Min = 0;

			if(x + 1 < Width) Min = min(*Pixel, *(Pixel + 1) + 1); //*(Pixel + 1) is Map[x + 1][y].
			if(y + 1 < Length) Min = min(*Pixel, *(Pixel + Width) + 1); //*(Pixel + Width) is Map[x][y + 1].
			if(Min > 0xFF) Min = 0xFF;
			*Pixel = Min;
		}
	}
}

void MapZeroArea(uint8_t* Map, uint16_t Width, int32_t Posx, int32_t Posy) {
	int32_t Startx = Posx - 20;
	int32_t Starty = Posy - 20;
	int32_t Endx = Posx + 20;
	int32_t Endy = Posy + 20;

	if(Startx < 0) Startx = 0;
	if(Starty < 0) Starty = 0;
	if(Endx >= Width) Endx = Width;
	if(Endy >= Width) Endy = Width;

	for(int x = Startx; x < Endx; ++x) {
		for(int y = Starty; y < Endy; ++y) {
			Map[y * Width + x] = 0;
		}
	}
}

uint8_t MapGravBest(uint8_t* Map, uint16_t Width, int32_t Posx, int32_t Posy, int32_t* Bestx, int32_t* Besty) {
	int32_t Startx = Posx - 20;
	int32_t Starty = Posy - 20;
	int32_t Endx = Posx + 20;
	int32_t Endy = Posy + 20;
	uint8_t BestScore = 0;
	uint8_t Score = 0;

	if(Startx < 0) Startx = 0;
	if(Starty < 0) Starty = 0;
	if(Endx >= Width) Endx = Width;
	if(Endy >= Width) Endy = Width;
	if(Bestx == NULL) return 0;
	if(Besty == NULL) return 0;

	for(int x = Startx; x < Endx; ++x) {
		for(int y = Starty; y < Endy; ++y) {
			Score = Map[y * Width + x];
			if(Score > BestScore) {
				*Besty = y;
				*Bestx = x;
				BestScore = Score;
			}
		}
	}
	return BestScore;
}

void CenterScreen(struct MapRenderer* Renderer, uint32_t x, uint32_t y) {
	int LeftPos = x - (Renderer->Screen.w / 2);
	int TopPos = y - (Renderer->Screen.h / 2);

	if(LeftPos < 0) LeftPos = 0;
	if(TopPos < 0)  TopPos = 0;
	Renderer->Screen.x = LeftPos;
	Renderer->Screen.y = TopPos;
}

/*void SetInf(const struct Settlement* Set, struct MapRenderer* Renderer, uint32_t* Score, uint32_t GovIdx) {
	static uint8_t SetScore[SET_SIZE] = {0, 50, 100, 200};
	static uint8_t Neighbors[TILE_SIZE] = {TILE_NORTHEAST, TILE_EAST, TILE_SOUTHEAST, TILE_SOUTHWEST, TILE_WEST, TILE_NORTHWEST};
	SDL_Point Pos= Set->Pos;
	uint32_t Radius = 20;
	uint32_t TileScore = SetScore[SettlementType(Set)];

	if(TileScore == 0) return;
	for(int i = 2; i <= Radius; ++i) {
		TileNeighbor(TILE_WEST, &Pos, &Pos);
		for(int k = 0; k < TILE_SIZE; ++k) {
			for(int j = 1; j < Radius; ++j) {
				if(Pos.x < 0 || Pos.y < 0) {
					TileNeighbor(Neighbors[k], &Pos, &Pos);
					continue;
				}
				Assert(Pos.x < Renderer->TileLength || Pos.x >= 0);
				Assert(Pos.y < Renderer->TileLength || Pos.y >= 0);
				Assert(((int32_t)TileScore) - j > 0);
				uint8_t TotalScore = TileScore - j;
				Assert(TotalScore <= TileScore);
				uint32_t* Tile = &Score[Pos.y * Renderer->TileLength + Pos.x];
				uint8_t CurrScore = (*Tile) & (0xFF);

				if(CurrScore != 0) {
					if(CurrScore < TotalScore) {
						*Tile = (GovIdx << 8) | (TotalScore - CurrScore);
					} else {
						*Tile = ((*Tile) & (~(uint32_t)0xFF)) | (CurrScore - TotalScore);
					}
				} else {
					*Tile = (GovIdx << 8) | (TotalScore);
				}
				TileNeighbor(Neighbors[k], &Pos, &Pos);
			}
		}
	}
}*/

/*void CalcInfluence(const struct Settlement** SetList, uint32_t GovSz, struct MapRenderer* Renderer) {
	uint32_t* Score = malloc(sizeof(uint32_t) * Renderer->TileLength * Renderer->TileLength);

	for(int i = 0; i < GovSz; ++i) {
		SetInf(SetList[i], Renderer, Score, i);
	}

	for(int i = 0; i < GovSz; ++i) {
		if((Score[SetList[i]->Pos.y * Renderer->TileLength + SetList[i]->Pos.x] >> 8) != i) {
			Assert(false);
			int foo = TileDistance(&SetList[i]->Pos, &SetList[(Score[SetList[i]->Pos.y * Renderer->TileLength + SetList[i]->Pos.x] >> 8)]->Pos);
			if(foo != 0) Assert(true);
		}
	}
}*/

int SetPosCmp(const void* One, const void* Two) {
	const struct Settlement* Sone = One;
	const struct Settlement* Stwo = Two;
	int Result = Sone->Pos.x - Stwo->Pos.x;
	
	if(Result == 0) return Result;
	return Sone->Pos.y - Stwo->Pos.y;
}

struct InfScore* CalcInfluence(const struct Settlement** List, uint32_t GovSz, struct MapRenderer* Renderer) {
	static uint8_t SetScore[SET_SIZE] = {0, 50, 100, 200};
	struct Settlement** SetList = calloc(sizeof(struct Settlement*), GovSz);
	struct InfScore* SetOwner = malloc(sizeof(struct InfScore) * GovSz);
	int TblSz = 40;
	uint32_t SetSz = 0;
	struct Settlement** OutList = alloca(sizeof(struct Settlement*) * TblSz);

	memset(SetOwner, 0, sizeof(int32_t) * GovSz);
	memcpy(SetList, List, sizeof(struct Settlement*) * GovSz);
	QuickSort((void**)SetList, GovSz, ObjectCmp);
	for(int i = 0; i < GovSz; ++i) {
		int32_t ScoreMod = SetScore[SettlementType(SetList[i])];

		if(ScoreMod == 0) continue;
		SettlementsInRadius(&g_GameWorld, &SetList[i]->Pos, 80, OutList, &SetSz, TblSz);
		for(int j = 0; j < SetSz; ++j) {
			if(OutList[j] == SetList[i]) continue;

			int Target = BinarySearchIdx(OutList[j], SetList, GovSz, ObjectCmp);
			int32_t TargetScore = SetScore[SettlementType(SetList[Target])];
			int Dist = TileDistance(&SetList[i]->Pos, &SetList[Target]->Pos);

			//if(SetScore[Target] == SetOwner[i]) continue;
			if(ScoreMod - Dist > TargetScore)  {
				SetOwner[Target].Score = ScoreMod - Dist;
				SetOwner[Target].SetId = i;
			}
		}
	}
	return SetOwner;
}
