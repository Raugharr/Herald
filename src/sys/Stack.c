/*
 * File: Stack.c
 * Author: David Brotz
 */

#include "Stack.h"

#include <stdlib.h>

struct Stack* CreateStack() {
	struct Stack* _Stack = (struct Stack*) malloc(sizeof(struct Stack));

	_Stack->Top = NULL;
	_Stack->Size = 0;
	return _Stack;
}

struct Stack* CopyStack(struct Stack* _Stack) {
	struct Stack* _NewStack = (struct Stack*) malloc(sizeof(struct Stack));
	struct StackNode* _StackNode = _Stack->Top;
	struct StackNode* _PrevNode = NULL;
	struct StackNode* _Node = NULL;

	_NewStack->Top = NULL;
	_NewStack->Size = _Stack->Size;
	if(_StackNode == NULL)
		return _NewStack;
	_Node = (struct StackNode*) malloc(sizeof(struct StackNode));
	_Node->Prev = NULL;
	_Node->Data = _StackNode->Data;
	_StackNode = _StackNode->Prev;
	while(_StackNode != NULL) {
		_PrevNode = (struct StackNode*) malloc(sizeof(struct StackNode));
		_PrevNode->Prev = NULL;
		_PrevNode->Data = _StackNode->Data;
		_Node->Prev = _PrevNode;
		_Node = _PrevNode;
		_StackNode = _StackNode->Prev;
	}
	return _NewStack;
}
void DestroyStack(struct Stack* _Stack) {
	struct StackNode* _Node = _Stack->Top;
	struct StackNode* _Temp = NULL;

	while(_Node != NULL) {
		_Temp = _Node;
		_Node = _Temp->Prev;
		free(_Temp);
	}
	free(_Stack);
}

void StackPush(struct Stack* _Stack, void* _Data) {
	struct StackNode* _Node = (struct StackNode*) malloc(sizeof(struct StackNode));

	_Node->Data = _Data;
	_Node->Prev = _Stack->Top;
	_Stack->Top = _Node;
}

void* StackPop(struct Stack* _Stack) {
	void* _Data = NULL;
	struct StackNode* _Temp = NULL;

	if(_Stack->Top == NULL)
		return NULL;
	_Temp = _Stack->Top;
	_Data = _Temp->Data;
	_Stack->Top = _Temp->Prev;
	free(_Temp);
	return _Data;
}

void* StackGet(struct Stack* _Stack, int _Index) {
	struct StackNode* _Node = _Stack->Top;
	int i;

	if(_Index > 0) {
		if(_Index > _Stack->Size)
			return NULL;
		i = _Stack->Size - _Index;
		while(i >= _Index) {
			if(_Node == NULL)
				return NULL;
			_Node = _Node->Prev;
			--i;
		}
		return _Node->Data;
	} else if(_Index < 0) {
		if(_Index > -(_Stack->Size))
			return NULL;
		i = 0;
		while(i > _Index) {
			if(_Node == NULL)
				return NULL;
			_Node = _Node->Prev;
			--i;
		}
	}
	return NULL;
}

int StackNodeLen(const struct StackNode* _Node) {
	int _Size = 0;

	if(_Node == NULL)
		return 0;
	while(_Node != NULL) {
		++_Size;
		_Node = _Node->Prev;
	}
	return _Size;
}
