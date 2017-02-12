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
#define TILE_DIST (21) //cos(30) * (TILE_HEIGHT / 2)
#define TILE_HEIGHT_THIRD ((int)(TILE_HEIGHT * 0.75))
#define TILE_GRADIENT (0.57735026919) //(TILE_HEIGHT / 4) / (cos(30) * (TILE_HEIGHT / 2))
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
	//TODO: Remove and all all sprites in the same layer to be adjacent in memory then we can just query the memory position of the sprite to get the layer.
	uint8_t Layer;
};

struct Sprite* CtorSprite(struct Sprite* Sprite, struct Resource* Image, int Layer, const SDL_Point* TilePos);
void DtorSprite(struct Sprite* Sprite);
struct Sprite* CreateSprite(struct Resource* Image, int Layer, const SDL_Point* TilePos);
struct Sprite* CreateGameObject(struct MapRenderer* Renderer, struct Resource* Image, int Layer, const SDL_Point* TilePos);
struct Sprite* ConstructGameObject(struct Sprite* Sprite, struct MapRenderer* Renderer, struct Resource* Image, int Layer, const SDL_Point* TilePos);
void DestroySprite(struct Sprite* Sprite);

void SpriteOnDraw(SDL_Renderer* Renderer, const struct Sprite* Sprite, uint16_t ScreenX, uint16_t ScreenY);
void SpriteSetTilePos(struct Sprite* Sprite, const struct MapRenderer* Renderer, const SDL_Point* TilePos);
void GameObjectSetTile(struct Sprite* Sprite, const SDL_Point* Pos);

static inline void SpriteGetTilePos(const struct Sprite* Sprite, SDL_Point* Pos) {
	*Pos = Sprite->TilePos;
}

#endif
