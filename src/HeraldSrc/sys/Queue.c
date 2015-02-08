/*
 * Author: David Brotz
 * File: queue.c
 */

#include "Queue.h"

#include <stdlib.h>

struct Queue* CreateQueue(int _Size) {
	struct Queue* _Queue = (struct Queue*) malloc(sizeof(struct Queue));
	
	_Queue->Table = malloc(sizeof(int*) * _Size);
	_Queue->Start = 0;
	_Queue->Size = 0;
	_Queue->TblSize = _Size;
	return _Queue;
}

void DestroyQueue(struct Queue* _Queue) {
	free(_Queue->Table);
	free(_Queue);
}

void QueuePush(struct Queue* _Queue, void* _Data) {
	int _Pos = _Queue->Start + _Queue->Size;
	
	if(_Pos > _Queue->TblSize)
		_Pos -= _Queue->TblSize;
	_Queue->Table[_Pos] = _Data;
	++_Queue->Size;
}

void* QueuePop(struct Queue* _Queue) {
	void* _Ret = NULL;

	if(_Queue->Size == 0)
		return NULL;

	_Ret = _Queue->Table[_Queue->Start++];
	if(_Queue->Start > _Queue->TblSize)
		_Queue->Start = 0;
	--_Queue->Size;
	return _Ret;
}
