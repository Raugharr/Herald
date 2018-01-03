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
#define TaskId(Task) ((((void*)(Task)) - ((void*)g_TaskAllocator)) / g_TaskPool.TaskSz)

static struct TaskPool g_TaskPool;
/**
 * Pointer to the array where all Tasks are allocated from.
 * Each thread is given TASKPOOL_SZ space to allocate which is guranteed to not be touched
 * by any other thread. This is done to allow generating an id for a task to be trivial where the id
 * is simply its index in this allocator. 
 */
static void* g_TaskAllocator = NULL;
static SDL_TLSID g_ThreadQueue;

int TaskQueueRand(struct TaskQueue* Queue) {
	Queue->RandNum = (TASKQUEUE_MULT * Queue->RandNum + TASKQUEUE_INCR) % (1 << 31);
	return Queue->RandNum;
}

uint32_t TaskBack(const struct TaskQueue* Queue) {return Queue->Front + Queue->Size;}

void TaskQueuePush(struct TaskQueue* Queue, struct Task* Task) {
	while(SDL_AtomicTryLock(&Queue->Lock) == SDL_FALSE);
	Queue->Queue[TaskBack(Queue) & TASKPOOL_MASK] = Task;
	++Queue->Size;
	SDL_AtomicUnlock(&Queue->Lock);
}

struct Task* TaskQueuePop(struct TaskQueue* Queue) {
	struct Task* Task = NULL;
	uint32_t Back = TaskBack(Queue);

	while(SDL_AtomicTryLock(&Queue->Lock) == SDL_FALSE);
	if(Queue->Size <= 0) {
		SDL_AtomicUnlock(&Queue->Lock);
		return NULL;
	}
	if(SDL_AtomicGet(&Queue->Queue[Back - 1]->UnfinishedJobs) > 1) {
		for(int i = Back - 2; i >= Queue->Front; --i) {
			if(SDL_AtomicGet(&Queue->Queue[i]->UnfinishedJobs) == 1) {
				struct Task* Temp = Queue->Queue[Back - 1];

				Queue->Queue[Back - 1] = Queue->Queue[i];
				Queue->Queue[i] = Temp;
				goto task_found;
			}
		}
		SDL_AtomicUnlock(&Queue->Lock);
		return NULL;
	}
	task_found:
	--Queue->Size;
	Task = Queue->Queue[Back & TASKPOOL_MASK];
	SDL_AtomicUnlock(&Queue->Lock);
	return Task;
}

struct Task* TaskQueueSteal(struct TaskQueue* Queue) {
	struct Task* Task = NULL;
	uint32_t Back = TaskBack(Queue);

	while(SDL_AtomicTryLock(&Queue->Lock) == SDL_FALSE);
	if(Queue->Size <= 0) {
		SDL_AtomicUnlock(&Queue->Lock);
		return NULL;
	}
	if(SDL_AtomicGet(&Queue->Queue[Queue->Front]->UnfinishedJobs) > 1) {
		for(int i = Queue->Front + 1; i <=Back; ++i) {
			if(SDL_AtomicGet(&Queue->Queue[i]->UnfinishedJobs) == 1) {
				struct Task* Temp = Queue->Queue[Queue->Front];

				Queue->Queue[Back] = Queue->Queue[i];
				Queue->Queue[i] = Temp;
				goto task_found;	
			}
		}
		SDL_AtomicUnlock(&Queue->Lock);
		return NULL;
	}
	task_found:
	Task = Queue->Queue[Queue->Front & TASKPOOL_MASK];
	++Queue->Front;
	SDL_AtomicUnlock(&Queue->Lock);
	return Task;
}

struct Task* TaskQueueAlloc(struct TaskQueue* Queue) {
	unsigned int Idx = ++Queue->AllocSz;
	return (Queue->Allocator + (g_TaskPool.TaskSz * ((Idx - 1) & TASKPOOL_MASK)));
}

void InitTaskQueue(struct TaskQueue* Queue) {
	Queue->Queue = calloc(TASKPOOL_SZ, sizeof(struct Task*));
	Queue->Front = 0;
	Queue->Size = 0;
	Queue->Lock = 0;
	Queue->RandNum = SDL_GetTicks(); 
	Queue->AllocSz = 0;
}

struct Task* TaskPoolNext(struct TaskQueue* Queue) {
	struct Task* Task = NULL;
	
	Task = TaskQueuePop(Queue);
	if(Task == NULL) {
		int StealThread = (TaskQueueRand(Queue) % g_TaskPool.ThreadCt);

		if(&g_TaskPool.Queues[StealThread] == Queue)
			return NULL;
		Task = TaskQueueSteal(&g_TaskPool.Queues[StealThread]);
	}
	
	return Task;
}

void TaskFinish(struct Task* Task) {
	int QueueId = (((void*) Task - g_TaskAllocator)) / (TASKPOOL_SZ * g_TaskPool.TaskSz);
	struct TaskQueue* Queue = &g_TaskPool.Queues[QueueId];

	if((Queue->AllocSz - 1) == TaskId(Task)) {
		while(SDL_AtomicTryLock(&Queue->Lock) == SDL_FALSE);
		--Queue->AllocSz;
		SDL_AtomicUnlock(&Queue->Lock);
	}
	if(Task->Parent != NULL) {
		SDL_AtomicDecRef(&Task->Parent->UnfinishedJobs);
		if(SDL_AtomicGet(&Task->Parent->UnfinishedJobs) == 0) {
			TaskFinish(Task->Parent->Parent);
		}
	}
}

static inline void TaskRun(struct Task* Task) {
	Task->Callback(TaskId(Task), ((void**)Task->Padding)[0]);
	TaskFinish(Task);
}

int TaskPoolThread(struct TaskQueue* Queue) {
	struct Task* Task = NULL;

	SDL_TLSSet(g_ThreadQueue, Queue, NULL);
	while(g_TaskPool.IsAlive) {
		if((Task = TaskPoolNext(Queue)) == NULL) {
			SDL_Delay(2); //FIXME: Instead of being delayed go to sleep until waken by a thread condition.
			continue;
		}
		TaskRun(Task);
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
	g_TaskPool.IsAlive = false;
	for(int i = 0; i < g_TaskPool.ThreadCt; ++i) {
		SDL_WaitThread(g_TaskPool.Threads[i], NULL);
		free(g_TaskPool.Queues[i].Queue);
	}
	free(g_TaskPool.Queues);
	free(g_TaskPool.Threads);
}

int TaskPoolAdd(int ParentId, TaskFunc Callback, void* Data, size_t Size) {
	struct Task* Task = NULL;
	struct Task* Parent = NULL;
	struct TaskQueue* Queue = SDL_TLSGet(g_ThreadQueue);

	assert(Size <= g_TaskPool.DataSz);
	if(ParentId != TASK_NOPARENT) {
		assert(ParentId >= 0 && ParentId < TASKPOOL_SZ);
		Parent = (g_TaskAllocator + (ParentId * g_TaskPool.TaskSz));
	}
	while(Queue->AllocSz >= TASKPOOL_SZ) {
		RunTasks();
	}
	Task = TaskQueueAlloc(Queue);
	Task->Callback = Callback;
	Task->Parent = Parent;
	memcpy(Task->Padding, &Data, Size);
	SDL_AtomicSet(&Task->UnfinishedJobs, 1);
	if(Parent != NULL)
		SDL_AtomicIncRef(&Parent->UnfinishedJobs);
	return TaskId(Task);
}

void TaskPoolExecute(int Id) {
	struct TaskQueue* Queue = SDL_TLSGet(g_ThreadQueue);
	//struct Task* Task = NULL;

	TaskQueuePush(Queue, (g_TaskAllocator + (Id * g_TaskPool.TaskSz)));	
}

void RunTasks() {
	struct TaskQueue* Queue = SDL_TLSGet(g_ThreadQueue);
	struct Task* Task = NULL;

	while(g_TaskPool.IsAlive) {
		if((Task = TaskPoolNext(Queue)) == NULL) {
			return;
		}
		TaskRun(Task);
	}
}
