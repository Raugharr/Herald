/*
 * File: Array.h
 * Author: David Brotz
 */

#ifndef __ARRAY_H
#define __ARRAY_H

#include <inttypes.h>
#include <stddef.h>

#define ArrayInsertSort(Array, Data, Callback) ArrayInsert((Array), (Data)); InsertionSortPtr((Array)->Table, (Array)->Size, Callback)
#define ArrayInsertSort_S(Array, Data, Callback) ArrayInsert_S((Array), (Data)); InsertionSortPtr((Array)->Table, (Array)->Size, Callback)
#define QuickSort(Table, Count, Callback) QuickSort_Aux((Table), (Callback), ((Count) - 1))
#define ArrayResize(Array) ArrayGrow((Array), (Array)->TblSize * 2)
#define ArrayRandom(Array) CArrayRandom((Array)->Table, (Array)->Size)

typedef int(*CompCallback)(const void*, const void*);

struct Array {
	void** Table;
	uint32_t Size;
	uint32_t TblSize;
};

struct Array* CreateArray(int Size);
void CtorArray(struct Array* Array, int Size);
void DtorArray(struct Array* Array);
struct Array* CopyArray(const struct Array* Array);
void DestroyArray(struct Array* Array);

static inline int ArrayInsert(struct Array* Array, void* Data) {
	if(Array->Size >= Array->TblSize)
		return 0;
	Array->Table[Array->Size++] = Data;
	return 1;
}

void ArrayInsert_S(struct Array* Array, void* Data);
void ArraySet_S(struct Array* Array, void* Data, uint32_t Idx);
void ArrayRemove(struct Array* Array, int Index);
void ArrayRemoveC(struct Array* Array, void* Elem, CompCallback Callback);
void ArrayGrow(struct Array* Array, uint32_t Size);
/*
 * Shuffles all the elements in Table.
 */
void CArrayRandom(void* Table, uint32_t Size);

void InsertionSort(void* Table, int Count, CompCallback Callback, int SizeOf);
void InsertionSortPtr(void* Table[], size_t Count, CompCallback Callback);
void QuickSort_Aux(void* Table, CompCallback Callback, int Size);
/**
 * Returns the size of a NULL terminated array.
 */
int ArrayLen(const void* Table);
int NArrayCount(const void** restrict TblOne, const void** restrict TblTwo);
int NArrayExists(const void** restrict Tbl, const void* restrict Ptr);
void* BinarySearch(const void* Data, void* Table, int Size, CompCallback Callback);
void* LinearSearch(const void* Data, void* Table, int Size, CompCallback Callback);
#endif
