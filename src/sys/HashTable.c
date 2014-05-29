/*
 * File: HashTable.c
 * Author: David Brotz
 */

#include "HashTable.h"

#include <stdlib.h>
#include <string.h>

int Hash(const char* _Key) {
	int _Len = strlen(_Key);
	int _Hash = 0;
	int i;
	int _One = 63689;
	int _Two = 378551;

	for(i = 0; i < _Len; ++i) {
		_Hash = _Hash * _One + _Key[i];
		_One *= _Two;
	}

	/*for(i = 0; i < _Len; i++) {
		_Hash += _Key[i];
		_Hash += (_Hash << 10);
		_Hash += (_Hash >> 6);
	}
	_Hash += (_Hash << 3);
	_Hash += (_Hash >> 11);
	_Hash += (_Hash << 15);*/
	return (_Hash & 0x7FFFFFFF);
}

int Hash_Find(struct HashTable* _Hash, const char* _Key, void* _Pair) {
	int _Index = Hash(_Key);
	struct HashNode* _Node = _Hash->Table[_Index];

	if(_Hash->TblSize == 0)
		return 0;
	while(_Node != 0) {
		if(!strcmp(_Node->Key, _Key)) {
			_Pair = _Node->Pair;
			return 1;
		}
		_Node = _Node->Next;
	}
	return 0;
}

void Hash_Insert(struct HashTable* _Hash, const char* _Key, void* _Pair) {
	char* _Str = (char*)malloc(sizeof(char) * strlen(_Key));

	strcpy(_Str, _Key);
	struct HashNode* i = _Hash->Table[Hash(_Key)];
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
		_Hash->Table[Hash(_Key)] = i;
	}
	++_Hash->Size;
}

