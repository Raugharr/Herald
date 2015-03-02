/*
 * File: TaskPool.h
 * Author: David Brotz
 */
#ifndef __TASKPOOL_H
#define __TASKPOOL_H

#include "BinaryHeap.h"

typedef struct SDL_Thread SDL_Thread;
typedef struct SDL_con SDL_con;
typedef struct SDL_mutex SDL_mutex;

struct Task {
	int (*Callback)(void*, void*);
	void* DataOne;
	void* DataTwo;
	int StartTime;
};

struct TaskPool {
	struct BinaryHeap Schedule;
	struct MemoryPool* TaskMemPool;
	unsigned int Clock;
	int ThreadCt;
	int IsAlive;
	int Time;
	SDL_Thread** Threads;
	SDL_mutex* PoolMutex;
};

int TaskCmp(const void* _One, const void* _Two);

struct TaskPool* CreateTaskPool();
void DestroyTaskPool(struct TaskPool* _TaskPool);

struct Task* TaskPoolNext(struct TaskPool* _Pool);
void TaskPoolAdd(struct TaskPool* _Pool, int _StartTime, int (*_Callback)(void*, void*), void* _DataOne, void* _DataTwo);

#endif
