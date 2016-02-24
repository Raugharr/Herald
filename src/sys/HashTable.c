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
	int _One = 63689;
	int _Two = 378551;

	for(int i = 0; i < _Len; ++i) {
		_Hash = _Hash * _One + _Key[i];
		_One *= _Two;
	}

	return (_Hash & 0x7FFFFFFF);
}

struct HashTable* CreateHash(int _Size) {
	struct HashTable* _Hash = (struct HashTable*) malloc(sizeof(struct HashTable));

	_Hash->Table = calloc(_Size, sizeof(void*));
	_Hash->TblSize = _Size;
	_Hash->Size = 0;
	return _Hash;
}

void DestroyHash(struct HashTable* _Hash) {
	int i;
	
	for(i = 0; i < _Hash->TblSize; ++i) 
		if(_Hash->Table[i] != NULL) {
			free(_Hash->Table[i]->Key);
			free(_Hash->Table[i]);
		}
	free(_Hash->Table);
	free(_Hash);
}

struct HashNode* HashSearchNode(const struct HashTable* _Hash, const char* _Key) {
	int _FirstIndex = 0;
	struct HashNode* _Node = NULL;
	int _Index;

	if(_Key == NULL)
		return NULL;
	if(_Hash->TblSize == 0)
		return NULL;
	_FirstIndex = Hash(_Key) % _Hash->TblSize;
	_Node = _Hash->Table[_FirstIndex];
	if(_Node == NULL)
		return NULL;
	_Index = _FirstIndex;
	while(_Node != NULL && strcmp(_Node->Key, _Key) != 0) {
		++_Index;
		if(_Index >= _Hash->TblSize)
			_Index = 0;
		_Node = _Hash->Table[_Index];
		if(_Index == _FirstIndex)
			return NULL;
	}
	if(_Node == NULL)
		return NULL;
	return _Node;
}

void* HashSearch(const struct HashTable* _Hash, const char* _Key) {
	struct HashNode* _Node = HashSearchNode(_Hash, _Key);

	if(_Node == NULL)
		return NULL;
	return _Node->Pair;
}

void HashInsert(struct HashTable* _Hash, const char* _Key, void* _Pair) {
	char* _Str = NULL;
	int _HashVal = 0;
	int i;
	struct HashNode* _Node = NULL;

	if(_Key == NULL)
		return;
	_Str = (char*) calloc(strlen(_Key) + 1, sizeof(char));
	_HashVal = Hash(_Key) % _Hash->TblSize;
	strcpy(_Str, _Key);
	i = _HashVal;
	while(_Hash->Table[i] != NULL) {
		++i;
		if(i >= _Hash->TblSize)
			i = 0;
		if(i == _HashVal)
			return;
	}
	_Node = (struct HashNode*) malloc(sizeof(struct HashNode));
	_Node->Key = _Str;
	_Node->Pair = _Pair;
	_Hash->Table[i] = _Node;
	++_Hash->Size;
}

void HashClear(struct HashTable* _Hash) {
	memset(_Hash->Table, 0, _Hash->TblSize);
	_Hash->Size = 0;
}

int HashDelete(struct HashTable* _Hash, const char* _Key) {
	int _HashVal = 0;
	int i;

	if(_Key == NULL)
		return 0;
	_HashVal = Hash(_Key) % _Hash->TblSize;
	i = _HashVal;

	while(_Hash->Table[i] == NULL ||
			strcmp(_Key, _Hash->Table[i]->Key) != 0) {
		if(i >= _Hash->TblSize)
			i = 0;
		++i;
		if(i == _HashVal)
			return 0;
	}
	free(_Hash->Table[i]->Key);
	free(_Hash->Table[i]);
	_Hash->Table[i] = NULL;
	--_Hash->Size;
	return 1;
}

void HashDeleteAll(struct HashTable* _Hash, void(*_Callback)(void*)) {
	for(int i = 0; i < _Hash->Size; ++i) {
		if(_Hash->Table[i] == NULL)
			continue;
		_Callback(_Hash->Table[i]->Pair);
		_Hash->Table[i] = NULL;
	}
	_Hash->Size = 0;
}

struct HashItr* HashCreateItr(struct HashTable* _Hash) {
	struct HashItr* _Itr = NULL;

	for(int i = 0; i < _Hash->TblSize; ++i) {
		if(_Hash->Table[i] != NULL) {
			_Itr = (struct HashItr*) malloc(sizeof(struct HashItr));
			_Itr->Index = i;
			_Itr->Node = _Hash->Table[i];
			return _Itr;
		}
	}
	return NULL;
}

void HashDeleteItr(struct HashItr* _Itr) {
	free(_Itr);
}

struct HashItr* HashNext(struct HashTable* _Hash, struct HashItr* _Itr) {
	int i;

	for(i = _Itr->Index + 1; i < _Hash->TblSize; ++i) {
		if(_Hash->Table[i] != NULL) {
			_Itr->Index = i;
			_Itr->Node = _Hash->Table[i];
			return _Itr;
		}
	}
	return NULL;
}

struct HashItrCons* HashCreateItrCons(const struct HashTable* _Hash) {
	return (struct HashItrCons*)HashCreateItr((struct HashTable*)_Hash);
}
void HashDeleteItrCons(struct HashItrCons* _Itr) {
	free(_Itr);
}
struct HashItrCons* HashNextCons(const struct HashTable* _Hash, struct HashItrCons* _Itr) {
	return (struct HashItrCons*)HashNext((struct HashTable*)_Hash, (struct HashItr*)_Itr);
}
