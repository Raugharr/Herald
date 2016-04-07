/*
 * File: LinkedList.c
 * Author: David Brotz
 */

#include "LinkedList.h"

#include "Math.h"

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
			if(_Itr->Next == NULL)
				_List->Back = _Node;
			else {
				_Node->Next = _Itr->Next;
				_Node->Next->Prev = _Node;
			}
			_Node->Prev = _Itr;
			_Itr->Next = _Node;
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
	_List->Size = 0;
	_List->Front = NULL;
	_List->Back = NULL;
}

void LnkLstPushBack(struct LinkedList* _List, void* _Value) {
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

void LnkLstPushFront(struct LinkedList* _List, void* _Value) {
	struct LnkLst_Node* _Node = CreateLnkLstNode(_Value);

	if(_List->Front == NULL) {
		_List->Front = _Node;
		_List->Back = _Node;
		++_List->Size;
		return;
	}
	if(_List->Front == _List->Back) {
		_Node->Next = _List->Front;
		_Node->Prev = NULL;
		_List->Front->Prev = _Node;
		_List->Front = _Node;
		++_List->Size;
		return;
	}
	_Node->Next = _List->Front;
	_Node->Prev = NULL;
	_List->Front = _Node;
	++_List->Size;
}

void* LnkLstPopFront(struct LinkedList* _List) {
	struct LnkLst_Node* _Node = _List->Front;
	void* _Data = NULL;

	if(_Node == NULL)
		return NULL;
	_Data = _Node->Data;
	_List->Front = _List->Front->Next;
	if(_List->Front != NULL)
		_List->Front->Prev = NULL;
	free(_Node);
	--_List->Size;
	if(_List->Size == 0)
		_List->Back = NULL;
	return _Data;
}

void* LnkLstPopBack(struct LinkedList* _List) {
	struct LnkLst_Node* _Node = _List->Back;
	void* _Data = NULL;

	if(_Node == NULL)
		return NULL;
	_Data = _Node->Data;
	if(_List->Front != _Node)
		_List->Back->Next = NULL;
	_List->Back = _Node->Prev;
	free(_Node);
	--_List->Size;
	if(_List->Size == 0)
		_List->Front = NULL;
	return _Data;
}

void LnkLstInsertAfter(struct LinkedList* _List, struct LnkLst_Node* _Node, void* _Value) {
	struct LnkLst_Node* _NewNode = CreateLnkLstNode(_Value);

	if(_Node->Next == NULL) {
		if(_Node == _List->Back)
			_List->Back = _NewNode;
		++_List->Size;
		_Node->Next = _NewNode;
		_NewNode->Next = NULL;
		_NewNode->Prev = _Node;
		return;
	}
	++_List->Size;
	_Node->Next->Prev = _NewNode;
	_NewNode->Next = _Node->Next;
	_Node->Next = _NewNode;
	_NewNode->Prev = _Node;
}

void LnkLstInsertBefore(struct LinkedList* _List, struct LnkLst_Node* _Node, void* _Value) {
	struct LnkLst_Node* _NewNode = CreateLnkLstNode(_Value);

	if(_Node->Prev == NULL) {
		if(_Node == _List->Front)
			_List->Front = _NewNode;
		++_List->Size;
		_Node->Prev = _NewNode;
		_NewNode->Next = _Node;
		_NewNode->Prev = NULL;
		return;
	}
	++_List->Size;
	_Node->Prev->Next = _NewNode;
	_NewNode->Prev = _Node->Prev;
	_Node->Prev = _NewNode;
	_NewNode->Next = _Node;
}

void LnkLstRemove(struct LinkedList* _List, struct LnkLst_Node* _Node) {
	struct LnkLst_Node* _Prev = NULL;

	if(_List->Size == 1) {
		_List->Front = NULL;
		_List->Back = NULL;
		_List->Size = 0;
		free(_Node);
		return;
	}
	_Prev = _Node->Prev;
	if(_List->Front == _Node) {
		_List->Front = _Node->Next;
		_List->Front->Prev = NULL;
	} else if(_List->Back == _Node) {
		_List->Back = _Node->Prev;
		_List->Back->Next = NULL;
	} else if(_Prev != NULL) {
		_Prev->Next = _Node->Next;
		_Prev->Next->Prev = _Prev;
	}
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

void LnkLstMerge(struct LinkedList* _List, const struct LinkedList* _Other) {
	struct LnkLst_Node* _Itr = _Other->Front;

	while(_Itr != NULL) {
		LnkLstPushBack(_List, _Itr->Data);
		_Itr = _Itr->Next;
	}
}

void LnkLstCatNode(struct LinkedList* _List, struct LnkLst_Node* _Node) {
	while(_Node != NULL) {
		LnkLstPushBack(_List, _Node->Data);
		_Node = _Node->Next;
	}
}

void* LnkLstRandom(struct LinkedList* _List) {
	struct LnkLst_Node* _Itr = _List->Front;
	int _RndIdx = Random(0, _List->Size - 1);
	int _Ct = 0;

	while(_Itr != NULL && _Ct < _RndIdx) {
		++_Ct;
		_Itr = _Itr->Next;
	}
	return (_Itr == NULL) ? (NULL) : (_Itr->Data);
}

void** LnkLstToList(const struct LinkedList* _List) {
	struct LnkLst_Node* _Node = _List->Front;
	void** _Array = calloc(_List->Size + 1, sizeof(void*));
	int _Idx = 0;

	while(_Node != NULL) {
		_Array[_Idx] = _Node->Data;
		++_Idx;
		_Node = _Node->Next;
	}
	return _Array;
}
