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
int Array_Insert(struct Array* _Array, void* _Data);

#endif
