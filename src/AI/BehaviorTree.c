/*
 * File: BehaviorTree.c
 * Author: David Brotz
 */

#include "BehaviorTree.h"

#include <stdlib.h>
#include <stdarg.h>

int BhvSelector(struct Behavior* _Bhv, struct Person* _Person, void* _Data) {
	int i;
	int _Ret = 0;
	struct Behavior* _Child = NULL;

	for(i = 0; i < _Bhv->Size; ++i) {
		_Child = _Bhv->Children[i];
		if(_Child->Size == 0)
			_Ret = _Child->Action(_Person, _Data);
		else
			_Ret = _Child->Callback(_Bhv, _Person, _Data);
		if(_Ret != 0)
			return _Ret;
	}
	return 0;
}

int BhvSequence(struct Behavior* _Bhv, struct Person* _Person, void* _Data) {
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

int BhvNot(struct Behavior* _Bhv, struct Person* _Person, void* _Data) {
	return !(_Bhv->Action(_Person, _Data));
}

struct Behavior* CreateBehavior(struct Behavior* _Parent, BhvAction _Run, int _Size, BhvCallback _Callback) {
	struct Behavior* _Bhv = NULL;

	_Bhv = (struct Behavior*) malloc(sizeof(struct Behavior));
	_Bhv->Callback = _Callback;
	_Bhv->Action = _Run;
	_Bhv->Children = calloc(_Size, sizeof(struct Behavior*));
	_Bhv->Size = _Size;
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

struct Behavior* CreateBhvComp(BhvCallback _Callback, struct Behavior* _Bhv, ...) {
	int _Size = 0;
	int i;
	va_list _List;
	struct Behavior* _Comp = NULL;
	struct Behavior* _Itr = _Bhv;

	if(_Bhv == NULL)
		return NULL;
	va_start(_List, _Bhv);
	if(_Itr != NULL) {
		_Size = 1;
		while((_Itr = va_arg(_List, struct Behavior*)) != NULL)
			++_Size;
	}
	_Comp = CreateBehavior(NULL, NULL, _Size, _Callback);
	va_start(_List, _Bhv);
	_Comp->Children[0] = _Bhv;
	for(i = 1; i < _Size; ++i) {
		_Comp->Children[i] = va_arg(_List, struct Behavior*);
	}
	return _Comp;
}

struct Behavior* CreateBhvNode(BhvAction _Action) {
	return CreateBehavior(NULL, _Action, 0, NULL);
}

int BHVRun(struct Behavior* _Bhv, struct Person* _Person, void* _Data) {
	if(_Bhv->Callback == NULL)
		return _Bhv->Action(_Person, _Data);
	return _Bhv->Callback(_Bhv, _Person, _Data);
}
