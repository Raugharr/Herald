/*
 * File: HashTable.h
 * Author: David Brotz
 */

#ifndef __HASHTABLE_H
#define __HASHTABLE_H

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
	int TblSize;
	int Size;
};

struct HashTable* CreateHash(int _Size);
void DestroyHash(struct HashTable* _Hash);

struct HashNode* HashSearchNode(const struct HashTable* _Hash, const char* _Key);
void* HashSearch(const struct HashTable* _Hash, const char* _Key);
void HashInsert(struct HashTable* _Hash, const char* _Key, void* _Pair);
void HashClear(struct HashTable* _Hash);
int HashDelete(struct HashTable* _Hash, const char* _Key);

struct HashItr* HashCreateItr(struct HashTable* _Hash);
void HashDeleteItr(struct HashItr* _Itr);
struct HashItr* HashNext(struct HashTable* _Hash, struct HashItr* _Itr);

struct HashItrCons* HashCreateItrCons(const struct HashTable* _Hash);
void HashDeleteItrCons(struct HashItrCons* _Itr);
struct HashItrCons* HashNextCons(const struct HashTable* _Hash, struct HashItrCons* _Itr);

#endif /* HASHTABLE_H */
