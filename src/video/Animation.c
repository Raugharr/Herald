/*
 * File: Animation.c
 * Author: David Brotz
 */

#include "Animation.c"

static LinkedList g_AnimationList = {0, NULL, NULL};
static SDL_Mutex* g_AnimationLock = NULL;

void AnimationNextFrame(struct Animation* _Animation, int _CurrTime) {
	++_Animation->CurrFrame;
	if(_Animation->CurrFrame > _Animation->Key[_Animation->CurrKey].FrameStop) {
		AnimationStop(_Animation);
	} else {
		_Animation->CurrTime = _CurrTime;
	}
}

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
	SDL_MutexLock(g_AnimationLock);
		do {
			_Animation = (struct Animation*) _Itr->Data;
			DestroyAnimation(_Animation);
			LnkLst_Remove(_List, _Itr);
			_Itr = _Itr->Next;
		} while(_Itr != NULL);
	SDL_MutexUnlock(g_AnimationLock);
}

void AnimationStart(struct Animation* _Animation, int _Key) {
	if(_Key >= _Animation->KeySz)
		return;
	_Animation->CurrFrame = _Animation->Keys[_Key].FrameStart;
	_Animation->CurrKey = _Key;
	_Animation->IsPlaying = 1;
}

void AnimationStop(struct Animation* _Animation) {
	_Animation->IsPlaying = 0;
	_Animation->IsRepeating = 0;
}
