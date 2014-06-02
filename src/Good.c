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
	_Good->Price = 0;
	return _Good;
}

struct Good* CopyGood(const struct Good* _Good) {
	struct Good* _NewGood = (struct Good*) malloc(sizeof(struct Good));

	_NewGood->Name = (char*) malloc(sizeof(char) * strlen(_Good->Name) + 1);
	_NewGood->Category = _Good->Category;
	_NewGood->Quantity = _Good->Quantity;
	_NewGood->Id = _Good->Id;
	_NewGood->Price = _Good->Price;
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
