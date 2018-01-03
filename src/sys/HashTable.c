/*
 * File: HashTable.c
 * Author: David Brotz
 */

#include "HashTable.h"

#include "Log.h"

#include <stdlib.h>
#include <string.h>

uint32_t HashFunc(const char* Key) {
	int Hash = 0;
	int One = 63689;
	int Two = 378551;

	for(int i = 0; Key[i] != '\0'; ++i) {
		Hash = Hash * One + Key[i];
		One *= Two;
	}

	return (Hash & 0x7FFFFFFF);
}

void CtorHashTable(struct HashTable* Hash, uint32_t Size) {
	Hash->Table = calloc(Size, sizeof(void*));
	Hash->TblSize = Size;
	Hash->Size = 0;
}


struct HashTable* CreateHash(uint32_t Size) {
	struct HashTable* Hash = (struct HashTable*) malloc(sizeof(struct HashTable));
	
	CtorHashTable(Hash, Size);
	return Hash;
}

void DestroyHash(struct HashTable* Hash) {
	for(int i = 0; i < Hash->TblSize; ++i) 
		if(Hash->Table[i] != NULL) {
			free(Hash->Table[i]->Key);
			free(Hash->Table[i]);
		}
	free(Hash->Table);
	free(Hash);
}

struct HashNode* HashSearchNode(const struct HashTable* Hash, const char* Key) {
	int FirstIndex = 0;
	struct HashNode* Node = NULL;
	int Index;

	if(Key == NULL)
		return NULL;
	if(Hash->TblSize == 0)
		return NULL;
	FirstIndex = HashFunc(Key) % Hash->TblSize;
	Node = Hash->Table[FirstIndex];
	if(Node == NULL)
		return NULL;
	Index = FirstIndex;
	while(Node != NULL && strcmp(Node->Key, Key) != 0) {
		++Index;
		if(Index >= Hash->TblSize)
			Index = 0;
		Node = Hash->Table[Index];
		if(Index == FirstIndex)
			return NULL;
	}
	if(Node == NULL)
		return NULL;
	return Node;
}

void* HashSearch(const struct HashTable* Hash, const char* Key) {
	struct HashNode* Node = HashSearchNode(Hash, Key);

	if(Node == NULL)
		return NULL;
	return Node->Pair;
}

static inline void HashInsertNode(struct HashTable* Hash, struct HashNode* Node, uint32_t Idx) {
	uint32_t i = 0;

	i = Idx;
	while(Hash->Table[i] != NULL) {
		++i;
		if(i >= Hash->TblSize)
			i = 0;
		if(i == Idx)
			return;
	}
	Hash->Table[i] = Node;
	++Hash->Size;
}

void HashInsert(struct HashTable* Hash, const char* Key, void* Pair) {
	char* Str = NULL;
	int HashVal = 0;
	struct HashNode* Node = NULL;
	
	if(Hash->TblSize == 0 || Key == NULL) return;
	if(Hash->TblSize <= Hash->Size) {
		struct HashNode** NewHash = calloc(Hash->TblSize * 2, sizeof(struct HashNode**));
		struct HashNode** OldHash = Hash->Table;
		uint32_t Size = Hash->TblSize;

		Hash->Size = 0;
		Hash->Table = NewHash;
		Hash->TblSize *= 2;
		for(int i = 0; i < Size; ++i) {
			if(OldHash[i] == NULL) continue;
			HashVal = HashFunc(OldHash[i]->Key) % Hash->TblSize;
			HashInsertNode(Hash, Node, HashVal);
		}
		free(OldHash);
	}
	HashVal = HashFunc(Key) % Hash->TblSize;
	Node = (struct HashNode*) malloc(sizeof(struct HashNode));
	Str = (char*) calloc(strlen(Key) + 1, sizeof(char));
	Node->Key = Str;
	Node->Pair = Pair;
	strcpy(Str, Key);
	HashInsertNode(Hash, Node, HashVal);
}

//FIXME: Does not free any memory.
void HashClear(struct HashTable* Hash) {
	memset(Hash->Table, 0, Hash->TblSize);
	Hash->Size = 0;
}

void HashResize(struct HashTable* Hash) {
	uint32_t OldTblSz = Hash->TblSize;
	struct HashNode** OldTbl = Hash->Table;
	uint32_t HashVal = 0;

	if(Hash->Table == NULL) {
		Hash->Table = calloc(64, sizeof(struct HashNode*));
		Hash->TblSize = 64;
		Hash->Size = 0;
		return;
	}
	Hash->Table = calloc(OldTblSz * 2, sizeof(struct HashNode*));
	Hash->TblSize = OldTblSz * 2;

	for(uint32_t i = 0; i < OldTblSz; ++i) {
		if(OldTbl[i] != NULL) {
			HashVal = HashFunc(OldTbl[i]->Key) % Hash->TblSize;
			HashInsertNode(Hash, OldTbl[i], HashVal);
		}
	}
}

int HashDelete(struct HashTable* Hash, const char* Key) {
	int HashVal = 0;
	int i;

	if(Key == NULL)
		return 0;
	HashVal = HashFunc(Key) % Hash->TblSize;
	i = HashVal;

	while(Hash->Table[i] == NULL ||
			strcmp(Key, Hash->Table[i]->Key) != 0) {
		if(i >= Hash->TblSize)
			i = 0;
		++i;
		if(i == HashVal)
			return 0;
	}
	free(Hash->Table[i]->Key);
	free(Hash->Table[i]);
	Hash->Table[i] = NULL;
	--Hash->Size;
	return 1;
}

void HashDeleteAll(struct HashTable* Hash, void(*Callback)(void*)) {
	for(int i = 0; i < Hash->Size; ++i) {
		if(Hash->Table[i] == NULL)
			continue;
		Callback(Hash->Table[i]->Pair);
		Hash->Table[i] = NULL;
	}
	Hash->Size = 0;
}

struct HashItr* HashCreateItr(struct HashTable* Hash) {
	struct HashItr* Itr = NULL;

	for(int i = 0; i < Hash->TblSize; ++i) {
		if(Hash->Table[i] != NULL) {
			Itr = (struct HashItr*) malloc(sizeof(struct HashItr));
			Itr->Index = i;
			Itr->Node = Hash->Table[i];
			return Itr;
		}
	}
	return NULL;
}

void HashDeleteItr(struct HashItr* Itr) {
	free(Itr);
}

struct HashItr* HashNext(struct HashTable* Hash, struct HashItr* Itr) {
	for(int i = Itr->Index + 1; i < Hash->TblSize; ++i) {
		if(Hash->Table[i] != NULL) {
			Itr->Index = i;
			Itr->Node = Hash->Table[i];
			return Itr;
		}
	}
	return NULL;
}

void HashItrRestart(struct HashTable* Hash, struct HashItr* Itr) {
	for(int i = 0; i < Hash->TblSize; ++i) {
		if(Hash->Table[i] != NULL) {
			Itr->Index = i;
			Itr->Node = Hash->Table[i];
			return;
		}
	}
}
void* HashItrData(const struct HashItr* Itr) {
	return Itr->Node->Pair;
}

struct HashItrCons* HashCreateItrCons(const struct HashTable* Hash) {
	return (struct HashItrCons*)HashCreateItr((struct HashTable*)Hash);
}
void HashDeleteItrCons(struct HashItrCons* Itr) {
	free(Itr);
}
struct HashItrCons* HashNextCons(const struct HashTable* Hash, struct HashItrCons* Itr) {
	return (struct HashItrCons*)HashNext((struct HashTable*)Hash, (struct HashItr*)Itr);
}
