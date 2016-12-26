/*
 * File: Array.h
 * Author: David Brotz
 */

#ifndef __ARRAY_H
#define __ARRAY_H

#include <inttypes.h>
#include <stddef.h>

#define ArrayInsertSort(_Array, _Data, _Callback) ArrayInsert((_Array), (_Data)); InsertionSortPtr((_Array)->Table, (_Array)->Size, _Callback)
#define ArrayInsertSort_S(_Array, _Data, _Callback) ArrayInsert_S((_Array), (_Data)); InsertionSortPtr((_Array)->Table, (_Array)->Size, _Callback)
#define QuickSort(_Table, _Count, _Callback) QuickSort_Aux((_Table), (_Callback), ((_Count) - 1))

typedef int(*CompCallback)(const void*, const void*);

struct Array {
	void** Table;
	int Size;
	int TblSize;
};

struct Array* CreateArray(int _Size);
void CtorArray(struct Array* _Array, int _Size);
void DtorArray(struct Array* _Array);
struct Array* CopyArray(const struct Array* _Array);
void DestroyArray(struct Array* _Array);

static inline int ArrayInsert(struct Array* _Array, void* _Data) {
	if(_Array->Size >= _Array->TblSize)
		return 0;
	_Array->Table[_Array->Size++] = _Data;
	return 1;
}

void ArrayInsert_S(struct Array* _Array, void* _Data);
void ArraySet_S(struct Array* _Array, void* _Data, uint32_t _Idx);
void ArrayRemove(struct Array* _Array, int _Index);
void ArrayResize(struct Array* _Array);

void InsertionSort(void* _Table, int _Count, CompCallback _Callback, int _SizeOf);
void InsertionSortPtr(void* _Table[], size_t _Count, CompCallback _Callback);
void QuickSort_Aux(void* _Table, CompCallback _Callback, int _Size);
/**
 * Returns the size of a NULL terminated array.
 */
int ArrayLen(const void* _Table);
int NArrayCount(const void** restrict _TblOne, const void** restrict _TblTwo);
int NArrayExists(const void** restrict _Tbl, const void* restrict _Ptr);
void* BinarySearch(const void* _Data, void* _Table, int _Size, CompCallback _Callback);
void* LinearSearch(const void* _Data, void* _Table, int _Size, CompCallback _Callback);
#endif
