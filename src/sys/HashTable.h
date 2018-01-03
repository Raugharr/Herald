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
	uint32_t Index;
	struct HashNode* Node;
};

struct HashItrCons {
	uint32_t Index;
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

void CtorHashTable(struct HashTable* Hash, uint32_t Size);
struct HashTable* CreateHash(uint32_t Size);
void DestroyHash(struct HashTable* Hash);

struct HashNode* HashSearchNode(const struct HashTable* Hash, const char* Key);
void* HashSearch(const struct HashTable* Hash, const char* Key);
void HashInsert(struct HashTable* Hash, const char* Key, void* Pair);
/**
 * Removes all elements from Hash.
 */
void HashClear(struct HashTable* Hash);
void HashResize(struct HashTable* Hash);
/**
 * Removes element Key from Hash.
 * Returns 1 if Key was removed and 0 if Key was not found.
 */
int HashDelete(struct HashTable* Hash, const char* Key);
void HashDeleteAll(struct HashTable* Hash, void(*_Callback)(void*));

struct HashItr* HashCreateItr(struct HashTable* Hash);
void HashDeleteItr(struct HashItr* Itr);
struct HashItr* HashNext(struct HashTable* Hash, struct HashItr* Itr);
void HashItrRestart(struct HashTable* Hash, struct HashItr* Itr);
void* HashItrData(const struct HashItr* Itr);

struct HashItrCons* HashCreateItrCons(const struct HashTable* Hash);
void HashDeleteItrCons(struct HashItrCons* Itr);
struct HashItrCons* HashNextCons(const struct HashTable* Hash, struct HashItrCons* Itr);

#endif /* HASHTABLE_H */
