/*
 * File: Animation.h
 * Author: David Brotz
 */
#ifndef __ANIMATION_H
#define __ANIMATION_H

#include <SDL2/SDL.h>

struct AnimationFrame {
	int Speed;
	SDL_Rect Rect;
};

struct AnimationFrameKey {
	int FrameStart;
	int FrameStop;
};

struct Animation {
	SDL_Texture* Image;
	SDL_Rect Rect; //Area of sprite to render.
	SDL_Point ScreenPos;
	int FrameSz;
	struct AnimationFrame* Frames;
	int AnimationSz;
	struct AnimationFrameKey Keys;
	int LastFramePlay; //Time the last frame was played.
	int IsPlaying;
	int CurrFrame;
	int CurrKey;
	int IsRepeating;
};

struct Animation* CreateAnimation(SDL_Texture* _Image);
void DestroyAnimation(struct Animation* _Animation);

void AnimationStart(struct Animation* _Animation, int _Key);
void AnimationStop(struct Animation* _Animation);

#endif
