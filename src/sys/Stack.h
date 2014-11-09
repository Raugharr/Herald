/*
 * File: Stack.h
 * Author: David Brotz
 */

#ifndef __STACK_H
#define __STACK_H

struct StackNode {
	struct StackNode* Prev;
	void* Data;
};

struct Stack {
	struct StackNode* Top;
	int Size;
};

struct Stack* CreateStack();
struct Stack* CopyStack(struct Stack* _Stack);
void DestroyStack(struct Stack* _Stack);

void StackPush(struct Stack* _Stack, void* _Data);
void* StackPop(struct Stack* _Stack);
void* StackGet(struct Stack* _Stack, int _Index);
int StackNodeLen(const struct StackNode* _Node);
struct StackNode* StackNodeConcat(struct StackNode* _Stack, const struct StackNode* _Cat);
static inline void* StackTop(struct Stack* _Stack) {return _Stack->Top->Data;}

#endif
