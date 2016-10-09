/*
 * File: HashTable.h
 * Author: David Brotz
 */

#ifndef __HASHTABLE_H
#define __HASHTABLE_H

#include <inttypes.h>

/*
 * FIXME: HashItr and HashItrCons should store their HashTable as a variable
 * instead of it always being passed.
 */
struct HashItr {
	int Index;
	struct HashNode* Node;
};

struct HashItrCons {
	int Index;
	const struct HashNode* const Node;
};

struct HashNode {
	void* Pair;
	char* Key;
};

struct HashTable {
	struct HashNode** Table;
	uint32_t TblSize;
	uint32_t Size;
};

void ConstructHashTable(struct HashTable* _Hash, uint32_t _Size);
struct HashTable* CreateHash(uint32_t _Size);
void DestroyHash(struct HashTable* _Hash);

struct HashNode* HashSearchNode(const struct HashTable* _Hash, const char* _Key);
void* HashSearch(const struct HashTable* _Hash, const char* _Key);
void HashInsert(struct HashTable* _Hash, const char* _Key, void* _Pair);
/**
 * Removes all elements from _Hash.
 */
void HashClear(struct HashTable* _Hash);
/**
 * Removes element _Key from _Hash.
 * Returns 1 if _Key was removed and 0 if _Key was not found.
 */
int HashDelete(struct HashTable* _Hash, const char* _Key);
void HashDeleteAll(struct HashTable* _Hash, void(*_Callback)(void*));

struct HashItr* HashCreateItr(struct HashTable* _Hash);
void HashDeleteItr(struct HashItr* _Itr);
struct HashItr* HashNext(struct HashTable* _Hash, struct HashItr* _Itr);
void HashItrRestart(struct HashTable* _Hash, struct HashItr* _Itr);
void* HashItrData(const struct HashItr* _Itr);

struct HashItrCons* HashCreateItrCons(const struct HashTable* _Hash);
void HashDeleteItrCons(struct HashItrCons* _Itr);
struct HashItrCons* HashNextCons(const struct HashTable* _Hash, struct HashItrCons* _Itr);

#endif /* HASHTABLE_H */
