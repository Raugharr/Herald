/*
 * File: Animation.c
 * Author: David Brotz
 */

#include "Animation.h"

#include "../sys/LinkedList.h"
#include "../sys/ResourceManager.h"

#include <stdlib.h>
#include <memory.h>
#include <SDL2/SDL_thread.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

static struct LinkedList g_AnimationList = {0, NULL, NULL};
static SDL_mutex* g_AnimationLock = NULL;

void AnimationNextFrame(struct Animation* _Animation, int _CurrTime) {
	++_Animation->CurrFrame;
	if(_Animation->CurrFrame > _Animation->Keys[_Animation->CurrKey].FrameStop) {
		if(_Animation->IsRepeating != 0) {
			_Animation->CurrFrame = _Animation->Keys[_Animation->CurrKey].FrameStart;
		} else {
			_Animation->LastFramePlay = _CurrTime;
			AnimationStop(_Animation);
			return;
		}
	} else {
		_Animation->LastFramePlay = _CurrTime;
	}
	_Animation->Rect = _Animation->Frames[_Animation->CurrFrame].Rect;
}

/*
 * The user must be able to call AnimationStart and AnimationStop when this thread is being executed.
 */
void AnimationQueue() {
	struct LinkedList* _List = NULL;
	struct LnkLst_Node* _Itr = NULL;
	struct Animation* _Animation = NULL;
	int _Time = SDL_GetTicks();

	while(_Itr != NULL) {
		_Animation = (struct Animation*) _Itr->Data;
		if(_Time - _Animation->LastFramePlay >= _Animation->Frames[_Animation->CurrFrame].Speed) {
			AnimationNextFrame(_Animation, _Time);
		}
		_Itr = _Itr->Next;
	}
	_Itr = _List->Front;
	if(_List->Size == 0)
		return;
	SDL_LockMutex(g_AnimationLock);
		do {
			_Animation = (struct Animation*) _Itr->Data;
			DestroyAnimation(_Animation);
			LnkLst_Remove(_List, _Itr);
			_Itr = _Itr->Next;
		} while(_Itr != NULL);
	SDL_UnlockMutex(g_AnimationLock);
}

struct Animation* CreateAnimation(SDL_Texture* _Image, const SDL_Point* _ScreenPos, const struct AnimationFrameKey* _Keys, const struct AnimationFrame* _Frames) {
	struct Animation* _Animation = (struct Animation*) malloc(sizeof(struct Animation));

	_Animation->Image = _Image;
	_Animation->Rect.x = 0;
	_Animation->Rect.y = 0;
	_Animation->Rect.w = 0;
	_Animation->Rect.h = 0;
	_Animation->ScreenPos = *_ScreenPos;
	_Animation->FrameSz = 0;
	_Animation->Frames = _Frames;
	_Animation->AnimationSz = 0;
	_Animation->Keys = _Keys;
	_Animation->KeySz = 0;
	_Animation->LastFramePlay = 0; //Time the last frame was played.
	_Animation->IsPlaying = 0;
	_Animation->CurrFrame = 0;
	_Animation->CurrKey = 0;
	_Animation->IsRepeating = 0;
	return _Animation;
}

void DestroyAnimation(struct Animation* _Animation) {
	free(_Animation);
}

void AnimationStart(struct Animation* _Animation, const char* _Name) {
	int _Key = 0;

	for(; _Key < _Animation->KeySz; ++_Key)
		if(strcmp(_Animation->Keys[_Key].Name, _Name) == 0)
			goto start_anim;
	return;
	start_anim:
	_Animation->CurrFrame = _Animation->Keys[_Key].FrameStart;
	_Animation->CurrKey = _Key;
	_Animation->IsPlaying = 1;
}

void AnimationStop(struct Animation* _Animation) {
	_Animation->IsPlaying = 0;
	_Animation->IsRepeating = 0;
}
