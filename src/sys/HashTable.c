/*
 * File: HashTable.c
 * Author: David Brotz
 */

#include "HashTable.h"

#include <stdlib.h>
#include <string.h>

uint32_t Hash(const char* _Key) {
	int _Hash = 0;
	int _One = 63689;
	int _Two = 378551;

	for(int i = 0; _Key[i] != '\0'; ++i) {
		_Hash = _Hash * _One + _Key[i];
		_One *= _Two;
	}

	return (_Hash & 0x7FFFFFFF);
}

void ConstructHashTable(struct HashTable* _Hash, uint32_t _Size) {
	_Hash->Table = calloc(_Size, sizeof(void*));
	_Hash->TblSize = _Size;
	_Hash->Size = 0;
}


struct HashTable* CreateHash(uint32_t _Size) {
	struct HashTable* _Hash = (struct HashTable*) malloc(sizeof(struct HashTable));
	
	ConstructHashTable(_Hash, _Size);
	return _Hash;
}

void DestroyHash(struct HashTable* _Hash) {
	for(int i = 0; i < _Hash->TblSize; ++i) 
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

static inline void HashInsertNode(struct HashTable* _Hash, struct HashNode* _Node, uint32_t _Idx) {
	uint32_t i = 0;

	i = _Idx;
	while(_Hash->Table[i] != NULL) {
		++i;
		if(i >= _Hash->TblSize)
			i = 0;
		if(i == _Idx)
			return;
	}
	_Hash->Table[i] = _Node;
	++_Hash->Size;
}

void HashInsert(struct HashTable* _Hash, const char* _Key, void* _Pair) {
	char* _Str = NULL;
	int _HashVal = 0;
	struct HashNode* _Node = NULL;

	if(_Key == NULL || _Hash->TblSize <= _Hash->Size)
		return;
	_HashVal = Hash(_Key) % _Hash->TblSize;
	_Node = (struct HashNode*) malloc(sizeof(struct HashNode));
	_Str = (char*) calloc(strlen(_Key) + 1, sizeof(char));
	_Node->Key = _Str;
	_Node->Pair = _Pair;
	strcpy(_Str, _Key);
	HashInsertNode(_Hash, _Node, _HashVal);
}

//FIXME: Does not free any memory.
void HashClear(struct HashTable* _Hash) {
	memset(_Hash->Table, 0, _Hash->TblSize);
	_Hash->Size = 0;
}

void HashResize(struct HashTable* _Hash) {
	uint32_t _OldTblSz = _Hash->TblSize;
	struct HashNode** _OldTbl = _Hash->Table;
	uint32_t _HashVal = 0;

	if(_Hash->Table == NULL) {
		_Hash->Table = calloc(64, sizeof(struct HashNode*));
		_Hash->TblSize = 64;
		_Hash->Size = 0;
		return;
	}
	_Hash->Table = calloc(_OldTblSz * 2, sizeof(struct HashNode*));
	_Hash->TblSize = _OldTblSz * 2;

	for(uint32_t i = 0; i < _OldTblSz; ++i) {
		if(_OldTbl[i] != NULL) {
			_HashVal = Hash(_OldTbl[i]->Key) % _Hash->TblSize;
			HashInsertNode(_Hash, _OldTbl[i], _HashVal);
		}
	}
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
	for(int i = _Itr->Index + 1; i < _Hash->TblSize; ++i) {
		if(_Hash->Table[i] != NULL) {
			_Itr->Index = i;
			_Itr->Node = _Hash->Table[i];
			return _Itr;
		}
	}
	return NULL;
}

void HashItrRestart(struct HashTable* _Hash, struct HashItr* _Itr) {
	for(int i = 0; i < _Hash->TblSize; ++i) {
		if(_Hash->Table[i] != NULL) {
			_Itr->Index = i;
			_Itr->Node = _Hash->Table[i];
			return;
		}
	}
}
void* HashItrData(const struct HashItr* _Itr) {
	return _Itr->Node->Pair;
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
