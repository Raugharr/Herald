/*
 * File: BinaryHeap.h
 * Author: David Brotz
 */
#ifndef __BINARYHEAP_H
#define __BINARYHEAP_H

#define BinaryHeapClear(_Heap) ((_Heap)->Size = 0)
#define BinaryHeapLeft(_Index) ((_Index) * 2 + 1)
#define BinaryHeapRight(_Index) ((_Index) * 2 + 2)
#define BinaryHeapParent(_Index) (((_Index) - 1) / 2)
#define BinaryHeapIsNode(_Index, _Heap) ((_Index) < (_Heap)->Size && _Heap->Table[(_Index)] != NULL)
#define BinaryHeapPop(_Heap) BinaryHeapRemove((_Heap), 0)

#ifndef NULL
#define NULL ((void*)0)
#endif

struct BinaryHeap {
	void** Table;
	int TblSz;
	int Size;
	int (*Compare)(const void*, const void*);
};

struct BinaryHeap* CreateBinaryHeap(int _Size, int (*_ICallback)(const void*, const void*));
void DestroyBinaryHeap(struct BinaryHeap* _Heap);
void BinaryHeapInsert(struct BinaryHeap* _Heap, void* _Data);
void* BinaryHeapRemove(struct BinaryHeap* _Heap, int _Index);
static inline void* BinaryHeapTop(struct BinaryHeap* _Heap) {
	return (_Heap->Size > 0) ? (_Heap->Table[0]) : (NULL);
}

void BinaryHeapIncrease(struct BinaryHeap* _Heap, int _Index);

#endif
