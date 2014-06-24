/*
 * File: BehaviorTree.c
 * Author: David Brotz
 */

#include "BehaviorTree.h"

#include <stdlib.h>
#include <stdarg.h>

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
	return !_Bhv->Action(_Person, _Data);
}

struct Behavior* CreateBehavior(struct Behavior* _Parent, BHVAction _Run, int _Size, int(*_Callback)(struct Behavior*, struct Person*, void*)) {
	struct Behavior* _Bhv = NULL;

	_Bhv = (struct Behavior*) malloc(sizeof(struct Behavior));
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
	free(_Bhv->Children);
	free(_Bhv);
}

struct Behavior* CreateBHVComp(int _Type, struct Behavior* _Bhv, ...) {
	int _Size = 0;
	int i;
	va_list _List;
	struct Behavior* _Comp = NULL;
	struct Behavior* _Itr = NULL;

	if(_Bhv == NULL)
		return NULL;
	va_start(_List, _Bhv);
	if(_Itr != NULL) {
		_Size = 1;
		while((_Itr = va_arg(_List, struct Behavior*)) != NULL)
			++_Size;
	}
	switch(_Type) {
		case BHV_SELECTOR:
			_Comp = CreateBehavior(NULL, NULL, _Size, BHVSelector);
			break;
		case BHV_SEQUENCE:
			_Comp = CreateBehavior(NULL, NULL, _Size, BHVSequence);
			break;
		default:
			return NULL;
	}
	va_start(_List, _Bhv);
	for(i = 0; i < _Size; ++i) {
		_Comp->Children[i] = va_arg(_List, struct Behavior*);
		_Comp->Children[i]->Parent = _Comp;
	}
	return _Comp;
}

struct Behavior* CreateBHVNode(BHVAction _Action) {
	return CreateBehavior(NULL, _Action, 0, NULL);
}

struct Behavior* CreateBHVD(int _Type, BHVAction _Action) {
	struct Behavior* _Bhv = NULL;

	switch(_Type) {
		case BHV_DNOT:
			_Bhv = CreateBehavior(NULL, NULL, 0, BHVDNot);
			break;
		default:
			return NULL;
	}
	_Bhv->Action = _Action;
	return _Bhv;
}

int BHVRun(struct Behavior* _Bhv, struct Person* _Person, void* _Data) {
	if(_Bhv->Callback == NULL)
		return _Bhv->Action(_Person, _Data);
	return _Bhv->Callback(_Bhv, _Person, _Data);
}
