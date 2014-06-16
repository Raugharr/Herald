/*
 * File: HashTable.h
 * Author: David Brotz
 */

#ifndef __HASHTABLE_H
#define __HASHTABLE_H

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

void* HashSearch(const struct HashTable* _Hash, const char* _Key);
void HashInsert(struct HashTable* _Hash, const char* _Key, void* _Pair);
int HashDelete(struct HashTable* _Hash, const char* _Key);

struct HashItr* HashCreateItr(struct HashTable* _Hash);
void HashDeleteItr(struct HashItr* _Itr);
struct HashItr* HashNext(struct HashTable* _Hash, struct HashItr* _Itr);

struct HashItrCons* HashCreateItrCons(const struct HashTable* _Hash);
void HashDeleteItrCons(struct HashItrCons* _Itr);
struct HashItrCons* HashNextCons(const struct HashTable* _Hash, struct HashItrCons* _Itr);

#endif /* HASHTABLE_H */
