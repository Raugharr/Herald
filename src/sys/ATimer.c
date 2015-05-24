/*
 * File: ATimer.c
 * Author: David Brotz
 */

#include "ATimer.h"

#include "RBTree.h"
#include "../Herald.h"

#include <stdlib.h>
#include <assert.h>

int NoThink(void* _Nothing) {
	return 1;
}

int ATimerICallback(const struct ATypeProto* _One, const struct ATypeProto* _Two) {
	return *((int*)_One->Data) - *((int*)_Two->Data);
}

int ATimerSCallback(const struct Object* _One, const struct ATypeProto* _Two) {
	return _One->Id - *((int*)_Two->Data);
}

struct ATType* CreateATType(int _Type, int(*_Callback)(void*), void(*_Delete)(void*)) {
	struct ATType* _ATType = (struct ATType*) malloc(sizeof(struct ATType));

	_ATType->Type = _Type;
	_ATType->Callback = _Callback;
	_ATType->Delete = _Delete;
	return _ATType;
}

struct ATType* CopyATType(struct ATType* _Type) {
	struct ATType* _ATType = (struct ATType*) malloc(sizeof(struct ATType));

	_ATType->Type = _Type->Type;
	_ATType->Callback = _Type->Callback;
	_ATType->Delete = _Type->Delete;
	return _ATType;
}

void DestroyATType(struct ATType* _Type) {
	free(_Type);
}

struct ATimer* CreateATimer() {
	struct ATimer* _Timer = (struct ATimer*) malloc(sizeof(_Timer));

	_Timer->Tree = CreateRBTree((int(*)(const void*, const void*))ATimerICallback, (int(*)(const void*, const void*))ATimerSCallback);
	_Timer->ATypes = calloc(ATT_SIZE, sizeof(struct ATType*));
	return _Timer;
}

struct ATimer* CopyATimer(struct ATimer* _Timer) {
	int i;
	struct ATimer* _NewTimer = (struct ATimer*) malloc(sizeof(_Timer));

	_NewTimer->Tree = CopyRBTree(_Timer->Tree);
	_NewTimer->ATypes = calloc(ATT_SIZE, sizeof(struct ATType*));

	for(i = 0; i < ATT_SIZE; ++i)
		_NewTimer->ATypes[i] = CopyATType(_Timer->ATypes[i]);
	return _Timer;
}

void DestroyATimer(struct ATimer* _Timer) {
	int i;

	for(i = 0; i < ATT_SIZE; ++i)
		DestroyATType(_Timer->ATypes[i]);
	free(_Timer->ATypes);
	DestroyRBTree(_Timer->Tree);
	free(_Timer);
}

void ATimerAddType(struct ATimer* _Timer, struct ATType* _Type) {
	assert(_Type->Type < ATT_SIZE);
	_Timer->ATypes[_Type->Type] = _Type;
}

void ATImerUpdate(struct ATimer* _Timer) {
/*	struct RBItrStack* _Stack = NULL;
	struct RBItrStack* _TempStk = NULL;
	struct RBItrStack* _Delete = NULL;
	struct RBNode* _Itr = NULL;
	struct ATypeProto* _Node = NULL;
	struct ATypeProto* _Temp = NULL;

	if(_Timer->Tree->Table == NULL)
		return;
	_Stack = RBStackPush(NULL, NULL);
	_Delete = RBStackPush(NULL, NULL);
	_Itr = _Timer->Tree->Table;

	while(_Itr != NULL) {
		_Node = _Itr->Data;
		while(_Node != NULL) {
			if(_Timer->ATypes[_Node->Type]->Callback(_Node) == 1) {
				_Temp = _Node;
				_Node = _Node->Next;
				if(_Temp->Prev != NULL)
					_Temp->Prev->Next = _Node;
				if(_Node != NULL) {
					_Node->Prev = _Temp;
				}
				if(_Temp == _Itr->Data) {
					if(_Node == NULL) {
						_Delete = RBStackPush(_Delete, _Itr);
						break;
					} else {
						_Timer->ATypes[_Node->Type]->Delete(_Temp->Data);
						_Itr->Data = _Node;
					}
				}
			} else {
				_Node = _Node->Next;
			}
		}
		if(_Itr->Right != NULL)
			_Stack = RBStackPush(_Stack, _Itr->Right);
		if(_Itr->Left != NULL)
			_Stack = RBStackPush(_Stack, _Itr->Left);
		_TempStk = _Stack;
		_Itr = _Stack->Node;
		_Stack = _Stack->Prev;
		free(_TempStk);
	}
	if(_Delete->Node == NULL) {
		free(_Delete);
		return;
	}
	_Itr = _Delete->Node;
	while(_Itr != NULL) {
		_Temp = (struct ATypeProto*)_Itr->Data;
		RBDeleteNode(_Timer->Tree, _Itr);
		_Timer->ATypes[_Temp->Type]->Delete(_Temp);
		_TempStk = _Delete;
		_Delete = _Delete->Prev;
		_Itr = _Delete->Node;
		free(_TempStk);
	}
	return;*/
}

void* ATimerSearch(struct ATimer* _Timer, struct Object* _Obj, int _Type) {
	struct ATypeProto* _Proto = RBSearch(_Timer->Tree, _Obj);

	while(_Proto != NULL && _Proto->Type != _Type)
		_Proto = _Proto->Next;
	return _Proto;
}

void ATimerInsert(struct ATimer* _Timer, void* _Data) {
	struct RBNode* _Node = RBInsertSearch(_Timer->Tree, _Data, _Data);
	struct ATypeProto* _Proto = NULL;

	if(_Node != NULL) {
		_Proto = _Node->Data;
		_Proto->Prev = _Data;
		((struct ATypeProto*)_Data)->Next = _Proto;
		((struct ATypeProto*)_Data)->Prev = NULL;
		_Node->Data = _Data;
	}
}

void ATimerRm(struct ATimer* _Timer, void* _Data) {
	struct RBNode* _Node = RBSearchNode(_Timer->Tree, _Data);
	struct ATypeProto* _Proto = (struct ATypeProto*)_Data;

	if(_Node == NULL)
		return;
	if(_Node->Data == _Proto) {
		_Node->Data = _Proto->Next;
		if(_Node->Data == NULL)
			RBDeleteNode(_Timer->Tree, _Node);
		else
			((struct ATypeProto*)_Node->Data)->Prev = NULL;
		goto end;
	}
	if(_Proto->Prev != NULL)
		_Proto->Prev->Next = _Proto->Next;
	if(_Proto->Next != NULL)
		_Proto->Next->Prev = _Proto->Prev;
	end:
	_Timer->ATypes[_Proto->Type]->Delete(_Data);
}

void ATimerRmNode(struct ATimer* _Timer, void* _Data) {
	struct RBNode* _Node = RBSearchNode(_Timer->Tree, _Data);
	struct ATypeProto* _Proto = NULL;
	struct ATypeProto* _Temp = NULL;

	if(_Node == NULL)
		return;
	_Proto = (struct ATypeProto*)_Node->Data;
	while(_Proto != NULL) {
		_Temp = _Proto;
		_Proto = _Proto->Next;
		_Timer->ATypes[_Temp->Type]->Delete(_Temp);
	}
	RBDeleteNode(_Timer->Tree, _Node);
}

void ATTimerRmAll(struct ATimer* _Timer) {
	while(_Timer->Tree->Size > 0) {
		ATimerRmNode(_Timer, _Timer->Tree->Table);
		RBDeleteNode(_Timer->Tree, _Timer->Tree->Table);
	}
}
