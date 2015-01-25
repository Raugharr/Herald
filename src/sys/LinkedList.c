/*
 * File: LinkedList.c
 * Author: David Brotz
 */

#include "LinkedList.h"

#include <stdlib.h>

struct LnkLst_Node* CreateLnkLstNode(void* _Data) {
	struct LnkLst_Node* _Node = (struct LnkLst_Node*) malloc(sizeof(struct LnkLst_Node));

	_Node->Data = _Data;
	_Node->Next = NULL;
	_Node->Prev = NULL;
	return _Node;
}

struct LinkedList* CreateLinkedList() {
	struct LinkedList* _List = (struct LinkedList*) malloc(sizeof(struct LinkedList));

	_List->Size = 0;
	_List->Front = NULL;
	_List->Back = NULL;
	return _List;
}
void DestroyLinkedList(struct LinkedList* _List) {
	LnkLstClear(_List);
	free(_List);
}

void LnkLstInsertPriority(struct LinkedList* _List, void* _Value, int (*_Callback)(const void*, const void*)) {
	struct LnkLst_Node* _Node = CreateLnkLstNode(_Value);
	struct LnkLst_Node* _Itr = NULL;

	if(_List->Front == NULL) {
		_List->Front = _Node;
		_List->Back = _Node;
		++_List->Size;
		return;
	}
	if(_Callback(_Value, _List->Front->Data) > 0) {
		_Node->Next = _List->Front;
		_Node->Prev = NULL;
		_List->Front = _Node;
		return;
	}
	_Itr = _List->Front;
	do {
		if(_Callback(_Value, _List->Front->Data) <= 0) {
			_Node->Next = _Itr->Next;
			_Node->Prev = _Itr;
			_Node->Next->Prev = _Node;
			_Itr->Next = _Node;
			if(_Itr->Next == NULL)
				_List->Back = _Node;
			++_List->Size;
			return;
		}
		_Itr = _Itr->Next;
	} while(_Itr != NULL);
	_List->Back->Next = _Node;
	_Node->Prev = _List->Back;
	_List->Back = _Node;
	++_List->Size;
}

void LnkLstClear(struct LinkedList* _List) {
	struct LnkLst_Node* _Node = _List->Front;
	struct LnkLst_Node* _Temp = NULL;

	while(_Node != NULL) {
		_Temp = _Node->Next;
		free(_Node);
		_Node = _Temp;
	}
}

void LnkLst_PushBack(struct LinkedList* _List, void* _Value) {
	struct LnkLst_Node* _Node = CreateLnkLstNode(_Value);

	_Node->Data = _Value;
	if(_List->Front == NULL) {
		_List->Front = _Node;
		_List->Back = _Node;
		++_List->Size;
		return;
	}
	if(_List->Front == _List->Back) {
		_List->Back = _Node;
		_List->Front->Next = _Node;
		_Node->Prev = _List->Front;
		++_List->Size;
		return;
	}
	_List->Back->Next = _Node;
	_Node->Prev = _List->Back;
	_List->Back = _Node;
	++_List->Size;
}

void* LnkLst_PopFront(struct LinkedList* _List) {
	struct LnkLst_Node* _Node = _List->Front;
	void* _Data = NULL;

	if(_Node == NULL)
		return NULL;
	_Data = _Node->Data;
	_List->Front = _List->Front->Next;
	_List->Front->Prev = NULL;
	free(_Node);
	--_List->Size;
	return _Data;
}

void LnkLst_Remove(struct LinkedList* _List, struct LnkLst_Node* _Node) {
	struct LnkLst_Node* _Prev = _Node->Prev;

	_Prev->Next = _Node->Next;
	_Prev->Next->Prev = _Prev;
	free(_Node);
	--_List->Size;
}

void* LnkLstSearch(struct LinkedList* _List, const void* _Data, int (*_Compare)(const void*, const void*)) {
	struct LnkLst_Node* _Node = _List->Front;

	while(_Node != NULL) {
		if(_Compare(_Data, _Node->Data) == 0)
			return _Node->Data;
		_Node = _Node->Next;
	}
	return NULL;
}

void LnkLst_CatNode(struct LinkedList* _List, struct LnkLst_Node* _Node) {
	while(_Node != NULL) {
		LnkLst_PushBack(_List, _Node->Data);
		_Node = _Node->Next;
	}
}
