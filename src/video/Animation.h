/*
 * File: Animation.h
 * Author: David Brotz
 */
#ifndef __ANIMATION_H
#define __ANIMATION_H

#include <SDL2/SDL.h>

typedef struct lua_State lua_State;
struct Resource;

struct AnimationFrame {
	int Speed;
	SDL_Rect Rect;
};

struct AnimationFrameKey {
	int FrameStart;
	int FrameStop;
	const char* Name;
};

struct Animation {
	SDL_Texture* Image;
	SDL_Rect Rect; //Area of sprite to render.
	SDL_Point ScreenPos;
	int FrameSz;
	const struct AnimationFrame* Frames;
	int AnimationSz;
	const struct AnimationFrameKey* Keys;
	int KeySz;
	int LastFramePlay; //Time the last frame was played.
	int IsPlaying;
	int CurrFrame;
	int CurrKey;
	int IsRepeating;
};

struct Animation* CreateAnimation(SDL_Texture* _Image, const SDL_Point* _ScreenPos, const struct AnimationFrameKey* _Keys, const struct AnimationFrame* _Frames);
void DestroyAnimation(struct Animation* _Animation);

void AnimationStart(struct Animation* _Animation, const char* _Name);
void AnimationStop(struct Animation* _Animation);

#endif
