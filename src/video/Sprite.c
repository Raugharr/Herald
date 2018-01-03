/*
 * File: Sprite.c
 * Author: David Brotz
 */

#include "Sprite.h"

#include "Tile.h"
#include "MapRenderer.h"
#include "QuadTree.h"
#include "Video.h"

#include "../sys/ResourceManager.h"

#include <stdlib.h>

struct Sprite* CtorSprite(struct Sprite* Sprite, struct Resource* Image, int Layer, const SDL_Point* TilePos) {
	SDL_Texture* Texture = ResourceGetData(Image);

	Sprite->TilePos = *TilePos;
	Sprite->Rect.x = 0;
	Sprite->Rect.y = 0;
	Sprite->Rect.w = 0;
	Sprite->Rect.h = 0;
	Sprite->SpritePos.w = Sprite->Rect.w;
	Sprite->SpritePos.h = Sprite->Rect.h;
	Sprite->Image = Image;
	Sprite->Layer = Layer;
	if(Texture == NULL) {
	//	free(Sprite);
		Log(ELOG_ERROR, "Error: Sprite cannot load texture.");
		return NULL;
	}
	SDL_QueryTexture(Texture, NULL, NULL, &Sprite->Rect.w, &Sprite->Rect.h);
	return Sprite;
}

#include "../World.h"

void DtorSprite(struct Sprite* Sprite) {
	QTRemoveNode(&g_GameWorld.MapRenderer->RenderArea[Sprite->Layer], &Sprite->TilePos, (void(*)(const void*, SDL_Point*))SpriteGetTilePos, Sprite);
	if(Sprite->Image) DestroyResource(Sprite->Image);
}

struct Sprite* CreateSprite(struct Resource* Image, int Layer, const SDL_Point* TilePos) {
	struct Sprite* Sprite = (struct Sprite*) malloc(sizeof(struct Sprite));

	return CtorSprite(Sprite, Image, Layer, TilePos);
}

void DestroySprite(struct Sprite* Sprite) {
	DtorSprite(Sprite);
	free(Sprite);
}

struct Sprite* CreateGameObject(struct MapRenderer* Renderer, struct Resource* Image, int Layer, const SDL_Point* TilePos) {
	struct Sprite* Sprite = (struct Sprite*) malloc(sizeof(struct Sprite));

	return ConstructGameObject(Sprite, Renderer, Image, Layer, TilePos);
}

struct Sprite* ConstructGameObject(struct Sprite* Sprite, struct MapRenderer* Renderer, struct Resource* Image, int Layer, const SDL_Point* TilePos) {
	if(CtorSprite(Sprite, Image, Layer, TilePos) == NULL)
		return NULL;
	MapTileRenderRect(Renderer, &Sprite->TilePos, &Sprite->SpritePos);
	QTInsertPoint(&Renderer->RenderArea[Layer], Sprite, &Sprite->TilePos);
	return Sprite;
}

void SpriteOnDraw(SDL_Renderer* Renderer, const struct Sprite* Sprite, uint16_t ScreenX, uint16_t ScreenY) {
	SDL_Rect Pos = {Sprite->SpritePos.x - ScreenX, Sprite->SpritePos.y - ScreenY, Sprite->SpritePos.w, Sprite->SpritePos.h};

	if(SDL_RenderCopy(Renderer, ResourceGetData(Sprite->Image), &Sprite->Rect, &Pos) != 0) {
		Log(ELOG_ERROR, "%s", SDL_GetError());
	}
}

void SpriteSetTilePos(struct Sprite* Sprite, const struct MapRenderer* Renderer, const SDL_Point* TilePos) {
	MapTileRenderRect(Renderer, TilePos, &Sprite->SpritePos);
	Sprite->TilePos = *TilePos;
}
