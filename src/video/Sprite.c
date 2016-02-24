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

struct Sprite* CreateSprite(struct Resource* _Image, int _Layer, const SDL_Point* _TilePos) {
	struct Sprite* _Sprite = (struct Sprite*) malloc(sizeof(struct Sprite));

	return ConstructSprite(_Sprite, _Image, _Layer, _TilePos);
}

struct Sprite* ConstructSprite(struct Sprite* _Sprite, struct Resource* _Image, int _Layer, const SDL_Point* _TilePos) {
	SDL_Texture* _Texture = ResourceGetData(_Image);

	if(_Texture == NULL) {
		free(_Sprite);
		return NULL;
	}
	_Sprite->Image = _Image;
	_Sprite->TilePos = *_TilePos;
	_Sprite->Rect.x = 0;
	_Sprite->Rect.y = 0;
	SDL_QueryTexture(_Texture, NULL, NULL, &_Sprite->Rect.w, &_Sprite->Rect.h);
	_Sprite->SpritePos.w = _Sprite->Rect.w;
	_Sprite->SpritePos.h = _Sprite->Rect.h;
	return _Sprite;
}

void DestroySprite(struct Sprite* _Sprite) {
	DestroyResource(_Sprite->Image);
	free(_Sprite);
}

struct Sprite* CreateGameObject(struct MapRenderer* _Renderer, struct Resource* _Image, int _Layer, const SDL_Point* _TilePos) {
	struct Sprite* _Sprite = (struct Sprite*) malloc(sizeof(struct Sprite));

	return ConstructGameObject(_Sprite, _Renderer, _Image, _Layer, _TilePos);
}

struct Sprite* ConstructGameObject(struct Sprite* _Sprite, struct MapRenderer* _Renderer, struct Resource* _Image, int _Layer, const SDL_Point* _TilePos) {
	if(ConstructSprite(_Sprite, _Image, _Layer, _TilePos) == NULL)
		return NULL;
	MapTileRenderRect(_Renderer, &_Sprite->TilePos, &_Sprite->SpritePos);
	QTInsertPoint(&_Renderer->RenderArea[_Layer], _Sprite, &_Sprite->TilePos);
	return _Sprite;
}

int SpriteOnDraw(const struct Sprite* _Sprite) {
	return SDL_RenderCopy(g_Renderer, ResourceGetData(_Sprite->Image), &_Sprite->Rect, &_Sprite->SpritePos);
}

void SpriteSetTilePos(struct Sprite* _Sprite, const struct MapRenderer* _Renderer, const SDL_Point* _TilePos) {
	MapTileRenderRect(_Renderer, _TilePos, &_Sprite->SpritePos);
	_Sprite->TilePos = *_TilePos;
}
