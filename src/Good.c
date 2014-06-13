/*
 * File: Good.c
 * Author: David Brotz
 */

#include "Good.h"

#include "Herald.h"

#include <stdlib.h>
#include <string.h>

static int g_GoodId = 0;

struct Good* CreateGood(const char* _Name, int _Category) {
	struct Good* _Good = (struct Good*) malloc(sizeof(struct Good));

	_Good->Name = (char*) malloc(sizeof(char) * strlen(_Name) + 1);
	_Good->Category = _Category;
	_Good->Quantity = 0;
	_Good->Id = ++g_GoodId;
	_Good->InputGoods.Size = 0;
	_Good->InputGoods.Front = NULL;
	_Good->InputGoods.Back = NULL;
	strcpy(_Good->Name, _Name);
	return _Good;
}

struct Good* CopyGood(const struct Good* _Good) {
	struct Good* _NewGood = (struct Good*) malloc(sizeof(struct Good));
	struct LnkLst_Node* _Itr = _Good->InputGoods.Front;

	_NewGood->Name = (char*) malloc(sizeof(char) * strlen(_Good->Name) + 1);
	_NewGood->Category = _Good->Category;
	_NewGood->Quantity = _Good->Quantity;
	_NewGood->Id = _Good->Id;
	strcpy(_NewGood->Name, _Good->Name);
	while(_Itr != NULL) {
		struct InputReq* _Req = CreateInputReq();
		_Req->Req = ((struct InputReq*)_Itr->Data)->Req;
		_Req->Quantity = ((struct InputReq*)_Itr->Data)->Quantity;
		LnkLst_PushBack(&_NewGood->InputGoods, _Req);
		_Itr = _Itr->Next;
	}
	return _NewGood;
}

void DestroyGood(struct Good* _Good) {
	struct LnkLst_Node* _Itr = _Good->InputGoods.Front;

	while(_Itr != NULL) {
		DestroyInputReq(_Itr->Data);
		_Itr = _Itr->Next;
	}
	free(_Good->Name);
	free(_Good);
}
