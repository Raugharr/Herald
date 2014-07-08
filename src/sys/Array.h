/*
 * File: Array.h
 * Author: David Brotz
 */

#ifndef __ARRAY_H
#define __ARRAY_H

struct Array {
	void** Table;
	int TblSize;
	int Size;
};

struct Array* CreateArray(int _Size);
void DestroyArray(struct Array* _Array);
int ArrayInsert(struct Array* _Array, void* _Data);
void ArrayInsert_S(struct Array* _Array, void* _Data);
void ArrayResize(struct Array* _Array);

void InsertionSort(void* _Table, int _Count, int(*_Callback)(const void*, const void*));
#endif
