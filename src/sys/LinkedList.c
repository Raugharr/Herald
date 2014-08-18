/*
 * File: LinkedList.c
 * Author: David Brotz
 */

#include "LinkedList.h"

#include <stdlib.h>

struct LnkLst_Node* CreateNode(void* _Data) {
	struct LnkLst_Node* _Node = (struct LnkLst_Node*) malloc(sizeof(struct LnkLst_Node));

	_Node->Data = _Data;
	_Node->Next = NULL;
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
	struct LnkLst_Node* _Node = _List->Front;
	struct LnkLst_Node* _Temp = NULL;

	while(_Node != NULL) {
		_Temp = _Node->Next;
		free(_Node);
		_Node = _Temp;
	}
	free(_List);
}

void LnkLst_PushBack(struct LinkedList* _List, void* _Value) {
	struct LnkLst_Node* _Node = CreateNode(_Value);

	_Node->Data = _Value;
	_Node->Next = NULL;
	if(_List->Front == NULL) {
		_List->Front = _Node;
		_List->Back = _Node;
		++_List->Size;
		return;
	}
	if(_List->Front == _List->Back) {
		_List->Back = _Node;
		_List->Front->Next = _Node;
		++_List->Size;
		return;
	}
	_List->Back->Next = _Node;
	_List->Back = _Node;
	++_List->Size;
}

void LnkLst_PopFront(struct LinkedList* _List) {
	struct LnkLst_Node* _Node = _List->Front;

	if(_Node == NULL)
		return;
	_List->Front = _List->Front->Next;
	free(_Node);
	--_List->Size;
}

void LnkLst_Remove(struct LinkedList* _List, struct LnkLst_Node* _Prev, struct LnkLst_Node* _Node) {
	_Prev->Next = _Node->Next;
	free(_Node);
	--_List->Size;
}

void LnkLst_CatNode(struct LinkedList* _List, struct LnkLst_Node* _Node) {
	struct LnkLst_Node* _Temp = NULL;

	while(_Node != NULL) {
		LnkLst_PushBack(_List, _Node);
		_Temp = _Node->Next;
		_Node = _Temp;
	}
}
