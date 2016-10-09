/*
 * File: Stack.c
 * Author: David Brotz
 */

#include "Stack.h"

#include <stdlib.h>

struct Stack* CreateStack(uint32_t _Size) {
	struct Stack* _Stack = (struct Stack*) malloc(sizeof(struct Stack));

	_Stack->Size = 0;
	_Stack->MaxSize = _Size;
	_Stack->Top = calloc(sizeof(void*), _Size);
	return _Stack;
}

void DestroyStack(struct Stack* _Stack) {
	free(_Stack->Top);
	free(_Stack);
}

void StackPush(struct Stack* _Stack, void* _Data) {
	if(_Stack->Size >= _Stack->MaxSize) {
		if(_Stack->MaxSize == 0) {
			_Stack->MaxSize = 1;
			_Stack->Top = calloc(sizeof(void*), 1);
		}
	}
	_Stack->Top[_Stack->Size++] = _Data;
}

void* StackPop(struct Stack* _Stack) {
	if(_Stack->Size <= 0)
		return NULL;
	--_Stack->Size;
	return _Stack->Top[_Stack->Size];
}

void* StackGet(struct Stack* _Stack, int _Index) {
	if(_Index > 0) {
		if(_Index >= _Stack->Size)
			return NULL;
		return _Stack->Top[_Index];
	} else if(_Index < 0) {
		if(_Stack->Size + _Index < 0)
			return NULL;
		return _Stack->Top[_Stack->Size + _Index];
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
