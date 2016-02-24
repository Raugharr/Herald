/*
 * File: BehaviorTree.c
 * Author: David Brotz
 */

#include "BehaviorTree.h"

//#include "../sys/HashTable.h"

#include <stdlib.h>
#include <stdarg.h>

/*
#define BHV_HASHSZ (8)
#define HASHTABLE_SIZE (32)

static struct HashTable g_BhvHash[BHV_HASHSZ];

void InitBehaviorEngine() {
	for(int i = 0; i < BHV_HASHSZ; ++i) {
		g_BhvHash[i].Size = 0;
		g_BhvHash[i].TblSz = HASHTABLE_SIZE;
		g_BhvHash[i].Table = calloc(HASHTABLE_SIZE, sizeof(struct HashTableNode*));
	}
}

void QuitBehaviorEngine() {
	for(int i = 0; i < BHV_HASHSZ; ++i) {
		free(g_BhvHash[i].Table);
	}
}
*/
int BhvSelector(const struct Behavior* _Bhv, struct Family* _Family, struct HashTable* _Vars) {
	int i;
	int _Ret = 0;
	struct Behavior* _Child = NULL;

	for(i = 0; i < _Bhv->Size; ++i) {
		_Child = _Bhv->Children[i];
		if(!BhvIsComposite(_Child)) {
			if(BhvIsNode(_Child)) {
				_Ret = _Child->Action(_Family, _Vars, ((struct BehaviorNode*)_Child)->Arguments, ((struct BehaviorNode*)_Child)->ArgSize);
			} else {
				_Ret = _Child->Action(_Family, _Vars, ((struct BehaviorNode*)_Child)->Arguments, ((struct BehaviorNode*)_Child)->ArgSize);
			}
		} else{
			_Ret = _Child->Callback(_Child, _Family, _Vars);
		}
		if(_Ret != 0)
			return _Ret;
	}
	return 0;
}

int BhvSequence(const struct Behavior* _Bhv, struct Family* _Family, struct HashTable* _Vars) {
	int i;
	int _Ret = 0;
	struct Behavior* _Child = NULL;

	for(i = 0; i < _Bhv->Size; ++i) {
		_Child = _Bhv->Children[i];
		if(!BhvIsComposite(_Child)) {
			if(BhvIsNode(_Child)) {
				_Ret = _Child->Action(_Family, _Vars, ((struct BehaviorNode*)_Child)->Arguments, ((struct BehaviorNode*)_Child)->ArgSize);
			} else {
				_Ret = _Child->Action(_Family, _Vars, ((struct BehaviorNode*)_Child)->Arguments, ((struct BehaviorNode*)_Child)->ArgSize);
			}
		} else {
			_Ret = _Child->Callback(_Child, _Family, _Vars);
		}
		if(_Ret == 0)
			return 0;
	}
	return 1;
}

int BhvNot(const struct Behavior* _Bhv, struct Family* _Family, struct HashTable* _Vars) {
	return !(_Bhv->Action(_Family, _Vars, ((struct BehaviorNode*)_Bhv)->Arguments, ((struct BehaviorNode*)_Bhv)->ArgSize));
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
	if(BhvIsComposite(_Bhv) != 0) {
		for(int i = 0; i < _Bhv->Size; ++i) {
			if(_Bhv->Children[i] != NULL)
				DestroyBehavior(_Bhv->Children[i]);
		}
	}
	free(_Bhv->Children);
	free(_Bhv);
}

struct BehaviorNode* CreateBehaviorNode(BhvAction _Action, int _ArgSize) {
	struct BehaviorNode* _Behavior = (struct BehaviorNode*) malloc(sizeof(struct Behavior));

	_Behavior->Action = _Action;
	_Behavior->Callback = NULL;
	_Behavior->ArgSize = _ArgSize;
	_Behavior->Arguments = calloc(_ArgSize, sizeof(struct Primitive));
	return _Behavior;
}

void DestroyBehaviorNode(struct BehaviorNode* _Behavior) {
	free(_Behavior->Arguments);
	free(_Behavior);
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

int BHVRun(const struct Behavior* _Bhv, struct Family* _Family, struct HashTable* _Vars) {
	if(_Bhv->Callback == NULL) {
		if(BhvIsNode(_Bhv)) {
			return _Bhv->Action(_Family, _Vars, ((struct BehaviorNode*)_Bhv)->Arguments, ((struct BehaviorNode*)_Bhv)->ArgSize);
		}
		return _Bhv->Action(_Family, _Vars, NULL, 0);
	}
	return _Bhv->Callback(_Bhv, _Family, _Vars);
}
