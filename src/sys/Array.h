/*
 * File: Array.h
 * Author: David Brotz
 */

#ifndef __ARRAY_H
#define __ARRAY_H

#define ArrayInsertSort(_Array, _Data, _Callback) ArrayInsert((_Array), (_Data)); InsertionSort((_Array)->Table, (_Array)->Size, _Callback, sizeof(int*))
#define ArrayInsertSort_S(_Array, _Data, _Callback) ArrayInsert_S((_Array), (_Data)); InsertionSort((_Array)->Table, (_Array)->Size, _Callback, sizeof(int*))
#define QuickSort(_Table, _Count, _Callback) QuickSort_Aux((_Table), (_Callback), 0, ((_Count) - 1))

typedef int(*CompCallback)(const void*, const void*);

struct Array {
	void** Table;
	int TblSize;
	int Size;
};

struct Array* CreateArray(int _Size);
struct Array* CopyArray(const struct Array* _Array);
void DestroyArray(struct Array* _Array);
int ArrayInsert(struct Array* _Array, void* _Data);
void ArrayInsert_S(struct Array* _Array, void* _Data);
void ArrayRemove(struct Array* _Array, int _Index);
void ArrayResize(struct Array* _Array);

void InsertionSort(void* _Table, int _Count, CompCallback _Callback, int _SizeOf);
void QuickSort_Aux(void* _Table, CompCallback _Callback, int _Left, int _Right);
/**
 * Returns the size of a NULL terminated array.
 */
int ArrayLen(const void* _Table);
int NArrayCount(const void** restrict _TblOne, const void** restrict _TblTwo);
int NArrayExists(const void** restrict _Tbl, const void* restrict _Ptr);
void* BinarySearch(const void* _Data, void* _Table, int _Size, CompCallback _Callback);
void* LinearSearch(const void* _Data, void* _Table, int _Size, CompCallback _Callback);
#endif
