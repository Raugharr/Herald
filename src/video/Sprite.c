/*
 * File: Sprite.c
 * Author: David Brotz
 */

#include "Sprite.h"

#include "Tile.h"

#include "MapRenderer.h"
#include "QuadTree.h"
#include "Video.h"

#include <stdlib.h>

struct Sprite* CreateSprite(struct Resource* _Image, int _Layer, const SDL_Point* _TilePos) {
	struct Sprite* _Sprite = (struct Sprite*) malloc(sizeof(struct Sprite));

	return ConstructSprite(_Sprite, _Image, _Layer, _TilePos);
}

struct Sprite* ConstructSprite(struct Sprite* _Sprite, SDL_Texture* _Image, int _Layer, const SDL_Point* _TilePos) {
	_Sprite->Image = _Image;
	_Sprite->TilePos = *_TilePos;
	//_Sprite->ScreenPos.x = _Sprite->TilePos.x * TILE_WIDTH;
	//_Sprite->ScreenPos.y = _Sprite->TilePos.y * TILE_HEIGHT;
	_Sprite->Rect.x = 0;
	_Sprite->Rect.y = 0;
	SDL_QueryTexture(_Sprite->Image, NULL, NULL, &_Sprite->Rect.w, &_Sprite->Rect.h);
	_Sprite->SpritePos.w = _Sprite->Rect.w;
	_Sprite->SpritePos.h = _Sprite->Rect.h;
	return _Sprite;
}

void DestroySprite(struct Sprite* _Sprite) {
	free(_Sprite);
}

struct Sprite* CreateGameObject(struct MapRenderer* _Renderer, SDL_Texture* _Image, int _Layer, const SDL_Point* _TilePos) {
	struct Sprite* _Sprite = (struct Sprite*) malloc(sizeof(struct Sprite));

	return ConstructGameObject(_Sprite, _Renderer, _Image, _Layer, _TilePos);
}

struct Sprite* ConstructGameObject(struct Sprite* _Sprite, struct MapRenderer* _Renderer, SDL_Texture* _Image, int _Layer, const SDL_Point* _TilePos) {
	ConstructSprite(_Sprite, _Image, _Layer, _TilePos);
	MapTileRenderRect(_Renderer, &_Sprite->TilePos, &_Sprite->SpritePos);
	QTInsertPoint(&_Renderer->RenderArea[_Layer], _Sprite, &_Sprite->TilePos);
	return _Sprite;
}

int SpriteOnDraw(const struct Sprite* _Sprite) {
	return SDL_RenderCopy(g_Renderer, ResourceGetData(_Sprite->Image), &_Sprite->Rect, &_Sprite->SpritePos);
}

void SpriteSetTilePos(struct Sprite* _Sprite, const struct MapRenderer* _Renderer, const SDL_Point* _TilePos) {
	MapTileRenderRect(_Renderer, _TilePos, &_Sprite->SpritePos);
}
