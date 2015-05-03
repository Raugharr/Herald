/*
 * File: MapRenderer.h
 * Author: David Brotz
 */
#ifndef __MAPRENDERER_H
#define __MAPRENDERER_H

#include "QuadTree.h"
#include "AABB.h"

struct Tile;
struct AABB;
typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_Renderer SDL_Renderer;

struct MapRenderer {
	struct Tile** Tiles;
	int TileLength;
	int TileArea;
	struct QuadTree RenderArea;
	struct AABB Screen;
	SDL_Texture* Grass;
	SDL_Texture* OddGrass;
	SDL_Texture* Selector;
};

struct MapRenderer* CreateMapRenderer(int _MapLength, struct Point* _RenderSize);
void MapLoad(struct MapRenderer* _Map);
struct Tile* ScreenToTile(struct MapRenderer* _Map, struct Point* _Screen);
struct Tile* GetAdjTile(struct MapRenderer* _Map, const struct Tile* _Tile, int _TileDir);
void MapRender(SDL_Renderer* _Renderer, struct MapRenderer* _Map);

#endif
