/*
 * File: HashTable.c
 * Author: David Brotz
 */

#include "HashTable.h"

#include <stdlib.h>
#include <string.h>

unsigned int Hash(const char* _Key) {
	int _Len = strlen(_Key);
	int _Hash = 0;
	int i;
	int _One = 63689;
	int _Two = 378551;

	for(i = 0; i < _Len; ++i) {
		_Hash = _Hash * _One + _Key[i];
		_One *= _Two;
	}

	return (_Hash & 0x7FFFFFFF);
}

int Hash_Find(struct HashTable* _Hash, const char* _Key, void** _Pair) {
	int _Index = 0;
	struct HashNode* _Node = NULL;

	if(_Key == NULL)
		return 0;
	if(_Hash->TblSize == 0)
		return 0;
	_Index = Hash(_Key) % _Hash->TblSize;
	_Node = _Hash->Table[_Index];

	while(_Node != 0) {
		if(!strcmp(_Node->Key, _Key)) {
			*_Pair = _Node->Pair;
			return 1;
		}
		_Node = _Node->Next;
	}
	return 0;
}

void Hash_Insert(struct HashTable* _Hash, const char* _Key, void* _Pair) {
	char* _Str = NULL;
	int _HashVal = 0;
	struct HashNode* i;

	if(_Key == NULL)
		return;
	_Str = (char*)malloc(sizeof(char) * strlen(_Key));
	_HashVal = Hash(_Key) % _Hash->TblSize;
	strcpy(_Str, _Key);
	i = _Hash->Table[_HashVal];
	if(i != 0) {
		while(1) {
			if(i->Next == 0) {
				struct HashNode* _Next = (struct HashNode*) malloc(sizeof(struct HashNode));
				i->Next = _Next;
				_Next->Key = _Str;
				_Next->Pair = _Pair;
				_Next->Next = 0;
				break;
			} else
				i = i->Next;
		};
	} else {
		i = (struct HashNode*) malloc(sizeof(struct HashNode));
		i->Key = _Str;
		i->Pair = _Pair;
		i->Next = 0;
		_Hash->Table[_HashVal] = i;
	}
	++_Hash->Size;
}

int Hash_Delete(struct HashTable* _Hash, const char* _Key) {
	int _HashVal = 0;
	struct HashNode* _Itr;
	struct HashNode* _Prev;

	if(_Key == NULL)
		return 0;
	_HashVal = Hash(_Key) % _Hash->TblSize;
	_Itr = _Hash->Table[_HashVal];

	if(strcmp(_Key, _Itr->Key) == 0) {
		free(_Itr->Key);
		_Hash->Table[_HashVal] = _Itr->Next;
		--_Hash->Size;
		return 1;
	}

	_Prev = _Itr;
	_Itr = _Itr->Next;
	while(strcmp(_Key, _Itr->Key) != 0) {
		_Prev = _Itr;
		_Itr = _Itr->Next;
	}
	_Prev->Next = _Itr->Next;
	free(_Itr->Key);
	--_Hash->Size;
	return 1;
}
