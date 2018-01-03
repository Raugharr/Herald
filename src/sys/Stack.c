/*
 * File: Stack.c
 * Author: David Brotz
 */

#include "Stack.h"

#include <stdlib.h>
#include <string.h>

struct Stack* CreateStack(uint32_t Size) {
	struct Stack* Stack = (struct Stack*) malloc(sizeof(struct Stack));

	Stack->Size = 0;
	Stack->MaxSize = Size;
	Stack->Top = calloc(sizeof(void*), Size);
	return Stack;
}

void DestroyStack(struct Stack* Stack) {
	free(Stack->Top);
	free(Stack);
}

void StackPush(struct Stack* Stack, void* Data) {
	if(Stack->Size >= Stack->MaxSize) {
		if(Stack->MaxSize == 0) {
			Stack->MaxSize = 16;
			Stack->Top = calloc(sizeof(void*), 16);
		} else {
			void* Temp = NULL;

			Stack->MaxSize = Stack->MaxSize * 2;
			Temp = realloc(Stack->Top, Stack->MaxSize * sizeof(void*));
			if(Temp == NULL) {
				/*calloc(sizeof(void*), Stack->MaxSize);
				memcpy(Temp, Stack->Top, sizeof(void*) * Stack->MaxSize);
				free(Stack->Top);
				Stack->Top = Temp;*/
				return;
			}
			Stack->Top = Temp;
		}
	}
	Stack->Top[Stack->Size++] = Data;
}

void* StackPop(struct Stack* Stack) {
	if(Stack->Size <= 0)
		return NULL;
	--Stack->Size;
	return Stack->Top[Stack->Size];
}

void* StackGet(struct Stack* Stack, int Index) {
	if(Index > 0) {
		if(Index >= Stack->Size)
			return NULL;
		return Stack->Top[Index];
	} else if(Index < 0) {
		if(Stack->Size + Index < 0)
			return NULL;
		return Stack->Top[Stack->Size + Index];
	}
	return NULL;
}
