/*
 * Author: David Brotz
 * File: queue.h
 */

#ifndef __QUEUE_H
#define __QUEUE_H

struct Queue {
	void** Table;
	int Start;
	int Size;
	int TblSize;
};

struct Queue* CreateQueue(int _Size);
void DestroyQueue(struct Queue* _Queue);
void QueuePush(struct Queue* _Queue, void* _Data);
void* QueuePop(struct Queue* _Queue);

#endif

