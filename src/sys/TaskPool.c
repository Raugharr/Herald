/*
 * File: TaskPool.c
 * Author: David Brotz
 */

#include "TaskPool.h"

#include "MemoryPool.h"

#include <stdlib.h>
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL.h>

int TaskCmp(const void* _One, const void* _Two) {
	return ((struct Task*)_Two)->StartTime - ((struct Task*)_One)->StartTime;
}

int TaskPoolThread(struct TaskPool* _TaskPool) {
	struct Task* _Task = NULL;
	int _Ret = 0;

	while(_TaskPool->IsAlive) {
		if((_Task = TaskPoolNext(_TaskPool)) == NULL) {
			SDL_Delay(2);
			continue;
		}
		//If the task is not completed continue it on the next tick.
		if(_Task->StartTime <= _TaskPool->Time && (_Ret = _Task->Callback(_Task->DataOne, _Task->DataTwo)) != 0) {
			_Task->StartTime += _Ret;
			SDL_LockMutex(_TaskPool->PoolMutex);
			BinaryHeapInsert(&_TaskPool->Schedule, _Task);
			SDL_UnlockMutex(_TaskPool->PoolMutex);
		} else {
			SDL_LockMutex(_TaskPool->PoolMutex);
			MemPoolFree(_TaskPool->TaskMemPool, _Task);
			SDL_UnlockMutex(_TaskPool->PoolMutex);
		}
	}
	return 1;
}

struct TaskPool* CreateTaskPool() {
	struct TaskPool* _TaskPool = (struct TaskPool*) malloc(sizeof(struct TaskPool));
	int i = 0;

	_TaskPool->ThreadCt = 2;
	_TaskPool->Schedule.Table = calloc(1024, sizeof(struct Task));
	_TaskPool->Schedule.TblSz = 1024;
	_TaskPool->Schedule.Size = 0;
	_TaskPool->Schedule.Compare = &TaskCmp;
	_TaskPool->TaskMemPool = CreateMemoryPool(sizeof(struct Task), 1000);
	_TaskPool->Threads = calloc(_TaskPool->ThreadCt, sizeof(SDL_Thread*));
	_TaskPool->IsAlive = 1;
	_TaskPool->Time = 0;
	_TaskPool->PoolMutex = SDL_CreateMutex();
	for(i = 0; i < _TaskPool->ThreadCt; ++i)
		_TaskPool->Threads[i] = SDL_CreateThread(((int(*)(void*))&TaskPoolThread), "TaskPoolThread", _TaskPool);
	return _TaskPool;
}

void DestroyTaskPool(struct TaskPool* _TaskPool) {
	int i = 0;

	_TaskPool->IsAlive = 0;
	for(i = 0; i < _TaskPool->ThreadCt; ++i)
		SDL_WaitThread(_TaskPool->Threads[i], NULL);
	free(_TaskPool->Threads);
	for(i = 0; i < _TaskPool->Schedule.Size; ++i)
		MemPoolFree(_TaskPool->TaskMemPool, _TaskPool->Schedule.Table[i]);
	free(_TaskPool->Schedule.Table);
	DestroyMemoryPool(_TaskPool->TaskMemPool);
	SDL_DestroyMutex(_TaskPool->PoolMutex);
	free(_TaskPool);
}

struct Task* TaskPoolNext(struct TaskPool* _Pool) {
	struct Task* _Task = NULL;

	SDL_LockMutex(_Pool->PoolMutex);
	if(_Pool->Schedule.Size > 0 && ((struct Task*)_Pool->Schedule.Table[0])->StartTime <= _Pool->Time)
		_Task = (struct Task*)BinaryHeapPop(&_Pool->Schedule);
	SDL_UnlockMutex(_Pool->PoolMutex);
	return _Task;
}

void TaskPoolAdd(struct TaskPool* _Pool, int _StartTime, int (*_Callback)(void*, void*), void* _DataOne, void* _DataTwo) {
	struct Task* _Task = NULL;

	SDL_LockMutex(_Pool->PoolMutex);
	while(_Pool->TaskMemPool->FreeBlocks == NULL) {
		SDL_Delay(2);
	}
	_Task = MemPoolAlloc(_Pool->TaskMemPool);
	_Task->StartTime = _StartTime;
	_Task->Callback = _Callback;
	_Task->DataOne = _DataOne;
	_Task->DataTwo = _DataTwo;
	BinaryHeapInsert(&_Pool->Schedule, _Task);
	SDL_UnlockMutex(_Pool->PoolMutex);
}
