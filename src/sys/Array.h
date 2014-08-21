/*
 * File: Array.h
 * Author: David Brotz
 */

#ifndef __ARRAY_H
#define __ARRAY_H

#define ArrayInsertSort(_Array, _Data, _Callback) ArrayInsert(_Array, _Data); InsertionSort(_Array->Table, _Array->Size, _Callback);
#define ArrayInsertSort_S(_Array, _Data, _Callback) ArrayInsert_S(_Array, _Data); InsertionSort(_Array->Table, _Array->Size, _Callback);
#define QuickSort(_Table, _Count, _Callback) QuickSort_Aux((_Table), (_Callback), 0, (_Count - 1))

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
void ArrayResize(struct Array* _Array);

void InsertionSort(void* _Table, int _Count, int(*_Callback)(const void*, const void*));
void QuickSort_Aux(void* _Table, int(*_Callback)(const void*, const void*), int _Left, int _Right);
int ArrayLen(const void* _Table);
void* BinarySearch(const void* _Data, void* _Table, int _Size, int(*_Callback)(const void*, const void*));
#endif
