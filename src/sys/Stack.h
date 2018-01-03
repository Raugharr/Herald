/*
 * File: Stack.h
 * Author: David Brotz
 */

#ifndef __STACK_H
#define __STACK_H

#include <inttypes.h>

struct StackNode {
	struct StackNode* Prev;
	void* Data;
};

struct Stack {
	void** Top;
	uint32_t Size;
	uint32_t MaxSize;
};

struct Stack* CreateStack();
struct Stack* CopyStack(struct Stack* _Stack);
void DestroyStack(struct Stack* _Stack);

void StackPush(struct Stack* _Stack, void* _Data);
void* StackPop(struct Stack* _Stack);
void* StackGet(struct Stack* _Stack, int _Index);
static inline void* StackTop(struct Stack* _Stack) {return (_Stack->Top);}

#endif

