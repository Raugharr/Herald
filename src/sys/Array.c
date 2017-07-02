/*
 * File: Array.c
 * Author: David Brotz
 */

#include "Array.h"

#include "Math.h"

#include <stdlib.h>
#include <string.h>
#include <alloca.h>

struct Array* CreateArray(int Size) {
	struct Array* Array = (struct Array*) malloc(sizeof(struct Array));

	CtorArray(Array, Size);
	return Array;
}

void CtorArray(struct Array* Array, int Size) {
	Array->Table = (Size == 0) ? (NULL) : (calloc(Size, sizeof(void*)));
	Array->TblSize = Size;
	Array->Size = 0;
}

void DtorArray(struct Array* Array) {
	free(Array->Table);
}

struct Array* CopyArray(const struct Array* Array) {
	struct Array* New = CreateArray(Array->Size);

	for(int i = 0; i < Array->Size; ++i)
		New->Table[i] = Array->Table[i];
	New->Size = Array->Size;
	return New;
}

void DestroyArray(struct Array* Array) {
	if(Array == NULL)
		return;
	DtorArray(Array);
	free(Array);
}

void ArrayInsert_S(struct Array* Array, void* Data) {
	if(Array->Size >= Array->TblSize) {
		ArrayResize(Array);
	}
	Array->Table[Array->Size++] = Data;
}

void ArraySet_S(struct Array* Array, void* Data, uint32_t Idx) {
	while(Array->Size >= Idx) {
		ArrayResize(Array);	
	}
	Array->Table[Idx] = Data;
	++Array->Size;
}

void ArrayRemove(struct Array* Array, int Index) {
	if(Index < 0 || Index >= Array->Size) return;
	if(Array->Size > 1) {
		Array->Table[Index] = Array->Table[Array->Size - 1];
		Array->Table[Array->Size - 1] = NULL;
	} else {
		Array->Table[Index] = NULL;
	}
	--Array->Size;
}

void ArrayRemoveC(struct Array* Array, void* Elem, CompCallback Callback) {
	int Min = 0;
	int Max = Array->Size - 1;
	int Mid = 0;
	int Result = 0;
	void* Table = Array->Table;

	if(Elem == NULL) return;
	while(Max >= Min) {
		Mid = Min + ((Max - Min) / 2);
		Result = Callback(Elem, *(void**)(Table + sizeof(void*) * Mid));
		if(Result < 0)
			Max = Mid - 1;
		else if(Result > 0)
			Min = Mid + 1;
		else 
			return ArrayRemove(Array, Mid);
	}
}

void ArrayGrow(struct Array* Array, uint32_t Size) {
	void* Temp = NULL;
	
	if(Array->Table == NULL) {
		Size = 32;	
		Temp = calloc(Size, sizeof(void*));
	} else {
		Size = Array->TblSize + Size;
		Temp = realloc(Array->Table, Size * sizeof(void*));
	}

	if(Temp == NULL)
		return;
	//free(Array->Table);
	Array->Table = Temp;
	Array->TblSize = Size;
}

void CArrayRandom(void* Table, uint32_t Size) {
	void* Temp = NULL;
	uint32_t Rand = 0;
	void** VTable = (void**)Table;

	for(int i = Size; i > 1; --i) {
		Rand = Random(0, i - 1);
		Temp = VTable[Rand];
		VTable[Rand] = VTable[i - 1];
		VTable[i - 1] = Temp;
	}
}

/*void InsertionSort(void* Table, int Count, CompCallback Callback, int SizeOf) {
	int j;
	void* Node = alloca(SizeOf);
	void* Off = alloca(SizeOf);

	if(Count <= 1)
		return;
	for(int Base = 1; Base < Count; ++Base) {
		memcpy(Node, (int**)(Table + SizeOf * Base), SizeOf);
		j = Base - 1;
		while(j >= 0 && Callback(Node, (void**)(Table + SizeOf * j)) < 0) {
			Off = Table + SizeOf * (j + 1);
			memcpy(Off,  (int**)(Table + SizeOf * j), SizeOf);
			--j;
		}
		Off = Table + SizeOf * (j + 1);
		memcpy(Off,  Node, SizeOf);
	}
}*/

#define Offset(Base, Size, Idx) ((Base) + (Size) * (Idx))
void InsertionSort(void* Table, int Count, CompCallback Callback, int SizeOf) {
	int j;
	void* Swap = alloca(SizeOf);

	if(Count <= 1)
		return;
	for(int Base = 1; Base < Count; ++Base) {
		memcpy(Swap, Offset(Table, SizeOf, Base), SizeOf);
		j = Base;
		while(j > 0 && Callback(Offset(Table, SizeOf, j - 1), Swap) > 0) {
			memcpy(Offset(Table, SizeOf, j), Offset(Table, SizeOf, j - 1), SizeOf);
			--j;
		}
		memcpy(Offset(Table, SizeOf, j), Swap, SizeOf);
	}
}

void InsertionSortPtr(void* Table[], size_t Count, CompCallback Callback) {
	int j;
	void* Swap = NULL;

	if(Count <= 1)
		return;
	for(int Base = 1; Base < Count; ++Base) {
		Swap = Table[Base];
		j = Base;
		while(j > 0 && Callback(Table[j - 1], Swap) > 0) {
			Table[j] =  Table[j - 1];
			--j;
		}
		Table[j] = Swap;
	}

}
void QuickSort_Aux(void* Table, CompCallback Callback, int Size) {
	/*
	int i = Left;
	int j = Right;
	const void* Node = NULL;
	const void** Swap = NULL;

	if(Left >= Right)
		return;
	Node = *(const void**)(Table + sizeof(int*) * (Left));
	do {
		while(Callback(*((const void**)(Table + sizeof(int*) * j)), Node) >= 0 && i < j)
			--j;
		if(i != j) {
			Swap = (const void**)(Table + sizeof(int*) * i);
			*_Swap = *(const void**)(Table + sizeof(int*) * j);
			++i;
		}
		while(Callback(*((const void**)(Table + sizeof(int*) * i)), Node) <= 0 && i < j)
			++i;
		if(i != j) {
			Swap = (const void**)(Table + sizeof(int*) * j);
			*_Swap = *(const void**)(Table + sizeof(int*) * i);
			--j;
		}
	} while(i < j);
	Swap = (const void**)(Table + sizeof(int*) * i);
	*_Swap = Node;
 	QuickSort_Aux(Table, Callback, Left, i - 1);
	QuickSort_Aux(Table, Callback, i + 1, Right);
	*/
}

int ArrayLen(const void* Table) {
	int Size = 0;

	if(Table == NULL)
		return 0;

	while(*((void**)Table) != NULL) {
		Table += sizeof(void*);
		++Size;
	}
	return Size;
}

int ArrayCount(const void** restrict TblOne, const void** restrict TblTwo) {
	int Count = 0;

	for(int i = 0; TblOne[i] != NULL; ++i) {
		for(int j = 0; TblTwo[j] != NULL; ++j) {
			Count += (TblOne[i] == TblTwo[j]);
		}
	}
	return Count;
}

int NArrayExists(const void** restrict Tbl, const void* restrict Ptr) {
	for(int i = 0; Tbl[i] != NULL; ++i) {
		if(Tbl[i] == Ptr)
			return 1;
	}
	return 0;
}

void* BinarySearch(const void* Data, void* Table, int Size, CompCallback Callback) {
	int Min = 0;
	int Max = Size - 1;
	int Mid = 0;
	int Result = 0;

	while(Max >= Min) {
		Mid = Min + ((Max - Min) / 2);
		Result = Callback(Data, *(void**)(Table + sizeof(void*) * Mid));
		if(Result < 0)
			Max = Mid - 1;
		else if(Result > 0)
			Min = Mid + 1;
		else
			return *(void**)(Table + sizeof(void*) * Mid);
	}
	return NULL;
}

void* LinearSearch(const void* Data, void* Table, int Size, CompCallback Callback) {
	for(int i = 0; i < Size; ++i) {
		if(Callback(Data, *(void**)Table) == 0)
			return *(void**)Table;
		Table = (void**)(Table + sizeof(void*));
	}
	return NULL;
}
