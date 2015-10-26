/*
 * File: Sprite.h
 * Author: David Brotz
 */
#ifndef __SPRITE_H
#define __SPRITE_H

#include <SDL2/SDL.h>

struct Resource;

/*
 * TODO: Move to a new file called GameSprite.
 */
#define TILE_WIDTH (42)
#define TILE_WIDTH_THIRD (TILE_WIDTH * 0.75)
#define TILE_HEIGHT (48)
#define TILE_HEIGHT_THIRD (TILE_HEIGHT * 0.75)
#define TILE_GRADIENT ((TILE_HEIGHT / 4) / (TILE_WIDTH / 2))
#define DestroyGameObject(_Object) DestroySprite(_Object)

struct MapRenderer;
typedef struct SDL_Texture SDL_Texture;

struct Sprite {
	struct Resource* Image;
	/*
	 * TODO: Rect is currently not implemented.
	 */
	SDL_Rect Rect; //Area of sprite to render.
	SDL_Rect SpritePos;//Where to render sprite.
	/*
	 * FIXME: TilePos should no longer be here and instead in an abstraction that uses Sprite.
	 */
	SDL_Point TilePos;
};

struct Sprite* CreateSprite(struct Resource* _Image, int _Layer, const SDL_Point* _TilePos);
struct Sprite* ConstructSprite(struct Sprite* _Sprite, struct Resource* _Image, int _Layer, const SDL_Point* _TilePos);
struct Sprite* CreateGameObject(struct MapRenderer* _Renderer, struct Resource* _Image, int _Layer, const SDL_Point* _TilePos);
struct Sprite* ConstructGameObject(struct Sprite* _Sprite, struct MapRenderer* _Renderer, struct Resource* _Image, int _Layer, const SDL_Point* _TilePos);
void DestroySprite(struct Sprite* _Sprite);

int SpriteOnDraw(const struct Sprite* _Sprite);
void SpriteSetTilePos(struct Sprite* _Sprite, const struct MapRenderer* _Renderer, const SDL_Point* _TilePos);
void GameObjectSetTile(struct Sprite* _Sprite, const SDL_Point* _Pos);

static inline void SpriteGetTilePos(const struct Sprite* _Sprite, SDL_Point* _Pos) {
	*_Pos = _Sprite->TilePos;
}

#endif
