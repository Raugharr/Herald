/*
 * File: Stack.h
 * Author: David Brotz
 */

#ifndef __STACK_H
#define __STACK_H

#include "Log.h"

#include <stdint.h>

#ifndef NULL
	#define NULL ((void*)0)
#endif

struct StackNode {
	struct StackNode* Prev;
	void* Data;
};

struct Stack {
	void** Top;
	uint32_t Size;
	uint32_t MaxSize;
};

struct Stack* CreateStack(uint32_t _Size);
void DestroyStack(struct Stack* _Stack);

void StackPush(struct Stack* _Stack, void* _Data);
void* StackPop(struct Stack* _Stack);
void* StackGet(struct Stack* _Stack, int _Index);
int StackNodeLen(const struct StackNode* _Node);

static inline void* StackTop(struct Stack* _Stack) {
	if(_Stack->Size == 0)
		return NULL;
	return _Stack->Top[_Stack->Size - 1];
}

#endif
