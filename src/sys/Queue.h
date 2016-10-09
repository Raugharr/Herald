/*
 * Author: David Brotz
 * File: queue.h
 */

#ifndef __QUEUE_H
#define __QUEUE_H

#include <stdint.h>

struct Queue {
	void** Table;
	uint32_t Start;
	uint32_t Size;
	uint32_t TblSize;
};

void InitQueue(struct Queue* _Queue, int _Size);
struct Queue* CreateQueue(int _Size);
void DestroyQueue(struct Queue* _Queue);
void QueuePush(struct Queue* _Queue, void* _Data);
void* QueuePop(struct Queue* _Queue);

#endif

