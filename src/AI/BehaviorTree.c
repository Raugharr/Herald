/*
 * File: BehaviorTree.c
 * Author: David Brotz
 */

#include "BehaviorTree.h"

#include <stdlib.h>

int BHVSelector(struct Behavior* _Bhv, struct Person* _Person, void* _Data) {
	int i;
	int _Ret = 0;
	struct Behavior* _Child = NULL;

	for(i = 0; i < _Bhv->Size; ++i) {
		_Child = _Bhv->Children[i];
		if(_Child->Size == 0)
			_Ret = _Child->Action(_Person, _Data);
		else
			_Ret = _Child->Callback(_Bhv, _Person, _Data);
		if(_Ret == 1)
			return 1;
	}
	return 0;
}

int BHVSequence(struct Behavior* _Bhv, struct Person* _Person, void* _Data) {
	int i;
	int _Ret = 0;
	struct Behavior* _Child = NULL;

	for(i = 0; i < _Bhv->Size; ++i) {
		_Child = _Bhv->Children[i];
		if(_Child->Callback == NULL)
			_Ret = _Child->Action(_Person, _Data);
		else
			_Ret = _Child->Callback(_Child, _Person, _Data);
		if(_Ret == 0)
			return 0;
	}
	return 1;
}

int BHVDNot(struct Behavior* _Bhv, struct Person* _Person, void* _Data) {
	return !_Bhv->Children[0]->Action(_Person, _Data);
}

struct Behavior* CreateBehavior(struct Behavior* _Parent, int(*_Run)(struct Person*, void*), int _Size, int(*_Callback)(struct Behavior*, struct Person*, void*)) {
	struct Behavior* _Bhv = NULL;
	int i;

	if(_Parent) {
		for(i = 0; i < _Parent->Size; ++i) {
			if(_Parent->Children[i] == NULL) {
				_Bhv = (struct Behavior*) malloc(sizeof(struct Behavior));
				_Parent->Children[i] = _Bhv;
				goto create;
			}
		}
		return NULL;
	}
	_Bhv = (struct Behavior*) malloc(sizeof(struct Behavior));
	create:
	_Bhv->Callback = _Callback;
	_Bhv->Action = _Run;
	_Bhv->Children = calloc(_Size, sizeof(struct Behavior*));
	_Bhv->Size = _Size;
	_Bhv->Parent = _Parent;
	return _Bhv;
}

void DestroyBehavior(struct Behavior* _Bhv) {
	int i;

	for(i = 0; i < _Bhv->Size; ++i)
		if(_Bhv->Children[i] != NULL)
			DestroyBehavior(_Bhv->Children[i]);
	free(_Bhv);
}

struct Behavior* CreateBHVComp(struct Behavior* _Parent, int _Type, int _Size) {
	switch(_Type) {
		case BHV_SELECTOR:
			return CreateBehavior(_Parent, NULL, _Size, BHVSelector);
		case BHV_SEQUENCE:
			return CreateBehavior(_Parent, NULL, _Size, BHVSequence);
		default:
			return NULL;
	}
	return NULL;
}

struct Behavior* CreateBHVNode(struct Behavior* _Parent, int(*_Action)(struct Person*, void*)) {
	return CreateBehavior(_Parent, _Action, 0, NULL);
}

struct Behavior* CreateBHVD(struct Behavior* _Parent, int _Type, int(*_Action)(struct Person*, void*)) {
	struct Behavior* _Bhv = NULL;

	switch(_Type) {
		case BHV_DNOT:
			_Bhv = CreateBehavior(_Parent, NULL, 1, BHVDNot);
			break;
		default:
			return NULL;
	}
	_Bhv->Children[0] = CreateBehavior(_Parent, _Action, 0, NULL);
	return _Bhv;
}

int BHVRun(struct Behavior* _Bhv, struct Person* _Person, void* _Data) {
	if(_Bhv->Size == 0)
		return _Bhv->Action(_Person, _Data);
	return _Bhv->Callback(_Bhv, _Person, _Data);
}
