/*
 * File: TaskPool.c
 * Author: David Brotz
 */

#include "TaskPool.h"

#include "MemoryPool.h"

#include <assert.h>
#include <stdlib.h>
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL.h>

#define TASKQUEUE_MULT (1103515245) 
#define TASKQUEUE_INCR (12345)
#define TaskId(_Task) ((((void*)(_Task)) - ((void*)g_TaskAllocator)) / g_TaskPool.TaskSz)

static struct TaskPool g_TaskPool;
/**
 * Pointer to the array where all Tasks are allocated from.
 * Each thread is given TASKPOOL_SZ space to allocate which is guranteed to not be touched
 * by any other thread. This is done to allow generating an id for a task to be trivial where the id
 * is simply its index in this allocator. 
 */
static void* g_TaskAllocator = NULL;
static SDL_TLSID g_ThreadQueue;

int TaskQueueRand(struct TaskQueue* _Queue) {
	_Queue->RandNum = (TASKQUEUE_MULT * _Queue->RandNum + TASKQUEUE_INCR) % (1 << 31);
	return _Queue->RandNum;
}

void TaskQueuePush(struct TaskQueue* _Queue, struct Task* _Task) {
	while(SDL_AtomicTryLock(&_Queue->Lock) == SDL_FALSE);
	_Queue->Queue[_Queue->Back & TASKPOOL_MASK] = _Task;
	++_Queue->Back;
	SDL_AtomicUnlock(&_Queue->Lock);
}

struct Task* TaskQueuePop(struct TaskQueue* _Queue) {
	struct Task* _Task = NULL;

	while(SDL_AtomicTryLock(&_Queue->Lock) == SDL_FALSE);
	if(_Queue->Back - _Queue->Front <= 0) {
		SDL_AtomicUnlock(&_Queue->Lock);
		return NULL;
	}
	if(SDL_AtomicGet(&_Queue->Queue[_Queue->Back - 1]->UnfinishedJobs) > 1) {
		for(int i = _Queue->Back - 2; i >= _Queue->Front; --i) {
			if(SDL_AtomicGet(&_Queue->Queue[i]->UnfinishedJobs) == 1) {
				struct Task* _Temp = _Queue->Queue[_Queue->Back - 1];

				_Queue->Queue[_Queue->Back - 1] = _Queue->Queue[i];
				_Queue->Queue[i] = _Temp;
				goto task_found;
			}
		}
		SDL_AtomicUnlock(&_Queue->Lock);
		return NULL;
	}
	task_found:
	--_Queue->Back;
	_Task = _Queue->Queue[_Queue->Back & TASKPOOL_MASK];
	SDL_AtomicUnlock(&_Queue->Lock);
	return _Task;
}

struct Task* TaskQueueSteal(struct TaskQueue* _Queue) {
	struct Task* _Task = NULL;

	while(SDL_AtomicTryLock(&_Queue->Lock) == SDL_FALSE);
	if(_Queue->Back - _Queue->Front <= 0) {
		SDL_AtomicUnlock(&_Queue->Lock);
		return NULL;
	}
	if(SDL_AtomicGet(&_Queue->Queue[_Queue->Front]->UnfinishedJobs) > 1) {
		for(int i = _Queue->Front + 1; i <=_Queue->Back; ++i) {
			if(SDL_AtomicGet(&_Queue->Queue[i]->UnfinishedJobs) == 1) {
				struct Task* _Temp = _Queue->Queue[_Queue->Front];

				_Queue->Queue[_Queue->Back] = _Queue->Queue[i];
				_Queue->Queue[i] = _Temp;
				goto task_found;	
			}
		}
		SDL_AtomicUnlock(&_Queue->Lock);
		return NULL;
	}
	task_found:
	_Task = _Queue->Queue[_Queue->Front & TASKPOOL_MASK];
	++_Queue->Front;
	SDL_AtomicUnlock(&_Queue->Lock);
	return _Task;
}

struct Task* TaskQueueAlloc(struct TaskQueue* _Queue) {
	unsigned int _Idx = ++_Queue->AllocSz;
	return (_Queue->Allocator + (g_TaskPool.TaskSz * ((_Idx - 1) & TASKPOOL_MASK)));
}

void InitTaskQueue(struct TaskQueue* _Queue) {
	_Queue->Queue = calloc(TASKPOOL_SZ, sizeof(struct Task*));
	_Queue->Front = 0;
	_Queue->Back = 0;
	_Queue->Lock = 0;
	_Queue->RandNum = SDL_GetTicks(); 
	_Queue->AllocSz = 0;
}

struct Task* TaskPoolNext(struct TaskQueue* _Queue) {
	struct Task* _Task = NULL;
	
	_Task = TaskQueuePop(_Queue);
	if(_Task == NULL) {
		int _StealThread = (TaskQueueRand(_Queue) % g_TaskPool.ThreadCt);

		if(&g_TaskPool.Queues[_StealThread] == _Queue)
			return NULL;
		_Task = TaskQueueSteal(&g_TaskPool.Queues[_StealThread]);
	}
	
	return _Task;
}

void TaskFinish(struct Task* _Task) {
	int _QueueId = (((void*) _Task - g_TaskAllocator)) / (TASKPOOL_SZ * g_TaskPool.TaskSz);
	struct TaskQueue* _Queue = &g_TaskPool.Queues[_QueueId];

	if((_Queue->AllocSz - 1) == TaskId(_Task)) {
		while(SDL_AtomicTryLock(&_Queue->Lock) == SDL_FALSE);
		--_Queue->AllocSz;
		SDL_AtomicUnlock(&_Queue->Lock);
	}
	if(_Task->Parent != NULL) {
		SDL_AtomicDecRef(&_Task->Parent->UnfinishedJobs);
		if(SDL_AtomicGet(&_Task->Parent->UnfinishedJobs) == 0) {
			TaskFinish(_Task->Parent->Parent);
		}
	}
}

static inline void TaskRun(struct Task* _Task) {
	_Task->Callback(TaskId(_Task), ((void**)_Task->Padding)[0]);
	TaskFinish(_Task);
}

int TaskPoolThread(struct TaskQueue* _Queue) {
	struct Task* _Task = NULL;

	SDL_TLSSet(g_ThreadQueue, _Queue, NULL);
	while(g_TaskPool.IsAlive) {
		if((_Task = TaskPoolNext(_Queue)) == NULL) {
			SDL_Delay(2); //FIXME: Instead of being delayed go to sleep until waken by a thread condition.
			continue;
		}
		TaskRun(_Task);
	}
	return 1;
}

void InitTaskPool() {
	g_TaskPool.ThreadCt = SDL_GetCPUCount();
	g_TaskPool.TaskSz = SDL_GetCPUCacheLineSize();
	g_TaskPool.DataSz = g_TaskPool.TaskSz - sizeof(struct Task);
	g_TaskPool.Threads = calloc(g_TaskPool.ThreadCt, sizeof(SDL_Thread*));
	g_TaskPool.Queues = calloc(g_TaskPool.ThreadCt, sizeof(struct TaskQueue));
	g_TaskPool.IsAlive = 1;
	g_TaskAllocator = calloc(TASKPOOL_SZ * g_TaskPool.ThreadCt, sizeof(struct Task) + g_TaskPool.DataSz);
	InitTaskQueue(&g_TaskPool.Queues[0]);
	g_ThreadQueue = SDL_TLSCreate();
	SDL_TLSSet(g_ThreadQueue, &g_TaskPool.Queues[0], NULL);
	g_TaskPool.Queues[0].Allocator = g_TaskAllocator;
	for(int i = 1; i < g_TaskPool.ThreadCt; ++i) {
		InitTaskQueue(&g_TaskPool.Queues[i]);
		g_TaskPool.Queues[i].Allocator = g_TaskAllocator + (TASKPOOL_SZ * i);
		g_TaskPool.Threads[i] = SDL_CreateThread(((int(*)(void*))&TaskPoolThread), "TaskPoolThread", &g_TaskPool.Queues[i]);
	}
}

void QuitTaskPool() {
	for(int i = 0; i < g_TaskPool.ThreadCt; ++i) {
		SDL_WaitThread(g_TaskPool.Threads[i], NULL);
		free(g_TaskPool.Queues[i].Queue);
	}
	free(g_TaskPool.Queues);
	free(g_TaskPool.Threads);
}

int TaskPoolAdd(int _ParentId, TaskFunc _Callback, void* _Data, size_t _Size) {
	struct Task* _Task = NULL;
	struct Task* _Parent = NULL;
	struct TaskQueue* _Queue = SDL_TLSGet(g_ThreadQueue);

	assert(_Size <= g_TaskPool.DataSz);
	if(_ParentId != TASK_NOPARENT) {
		assert(_ParentId >= 0 && _ParentId < TASKPOOL_SZ);
		_Parent = (g_TaskAllocator + (_ParentId * g_TaskPool.TaskSz));
	}
	while(_Queue->AllocSz >= TASKPOOL_SZ) {
		RunTasks();
	}
	_Task = TaskQueueAlloc(_Queue);
	_Task->Callback = _Callback;
	_Task->Parent = _Parent;
	memcpy(_Task->Padding, &_Data, _Size);
	SDL_AtomicSet(&_Task->UnfinishedJobs, 1);
	if(_Parent != NULL)
		SDL_AtomicIncRef(&_Parent->UnfinishedJobs);
	return TaskId(_Task);
}

void TaskPoolExecute(int _Id) {
	struct TaskQueue* _Queue = SDL_TLSGet(g_ThreadQueue);
	//struct Task* _Task = NULL;

	TaskQueuePush(_Queue, (g_TaskAllocator + (_Id * g_TaskPool.TaskSz)));	
}

void RunTasks() {
	struct TaskQueue* _Queue = SDL_TLSGet(g_ThreadQueue);
	struct Task* _Task = NULL;

	while(g_TaskPool.IsAlive) {
		if((_Task = TaskPoolNext(_Queue)) == NULL) {
			return;
		}
		TaskRun(_Task);
	}
}
