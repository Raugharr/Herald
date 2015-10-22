/*
 * File: Animation.h
 * Author: David Brotz
 */
#ifndef __ANIMATION_H
#define __ANIMATION_H

#include <SDL2/SDL.h>

typedef struct lua_State lua_State;
struct Resource;

#define ANIM_MAXFRAMES (128)
#define ANIM_MAXKEYS (32)

struct AnimationFrame {
	int Speed;
	SDL_Rect Rect;
};

struct AnimationFrameKey {
	int FrameStart;
	int FrameStop;
	const char* Name;
};

/*
 * Note: AnimationBase are created in VideoLua.c
 */
struct AnimationBase {
	const char* ImageName;
	struct AnimationFrame Frames[ANIM_MAXFRAMES];
	int FrameSz;
	struct AnimationFrameKey Keys[ANIM_MAXKEYS];
	int KeySz;
};

struct Animation {
	struct Resource* Image;
	SDL_Rect Rect; //Area of sprite to render.
	SDL_Point ScreenPos;
	const struct AnimationBase* Base;
	int LastFramePlay; //Time the last frame was played.
	int IsPlaying;
	int CurrFrame;
	int CurrKey;
	int IsRepeating;
};

struct Animation* CreateAnimation(const SDL_Point* _ScreenPos, const struct AnimationBase* _Base);
void DestroyAnimation(struct Animation* _Animation);

void DestroyAnimationBase(struct AnimationBase* _Base);

void AnimationStart(struct Animation* _Animation, const char* _Name);
void AnimationStop(struct Animation* _Animation);

#endif
