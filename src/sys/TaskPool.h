/*
 * File: TaskPool.h
 * Author: David Brotz
 */
#ifndef __TASKPOOL_H
#define __TASKPOOL_H

#include <SDL2/SDL.h>

#define TASKPOOL_SZ (1 << 11)
#define TASKPOOL_MASK (TASKPOOL_SZ - 1)
#define TASK_NOPARENT (-1)

struct Task;

typedef struct SDL_Thread SDL_Thread;
typedef struct SDL_con SDL_con;
typedef struct SDL_mutex SDL_mutex;
typedef void(*TaskFunc)(int, void*);

struct Task {
	TaskFunc Callback;
	struct Task* Parent;
	SDL_atomic_t UnfinishedJobs;
	uint8_t Padding[];
};

struct TaskQueue {
	struct Task** Queue;
	SDL_SpinLock Lock;
	int Front;
	int Back;
	int RandNum;
	unsigned int AllocSz;
	void* Allocator;
};

struct TaskPool {
	struct TaskQueue* Queues;
	SDL_Thread** Threads;
	size_t DataSz;
	size_t TaskSz;
	uint8_t ThreadCt;
	uint8_t IsAlive;
};

void InitTaskPool();
void QuitTaskPool();

/**
 * \note If no parent is desired use TASK_NOPARENT.
 */
int TaskPoolAdd(int _ParentId, TaskFunc _Callback, void* _Data, size_t _Size);
void TaskPoolExecute(int _Id);
void RunTasks();

#endif
