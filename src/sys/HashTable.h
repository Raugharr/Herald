/*
 * File: HashTable.h
 * Author: David Brotz
 */

#ifndef __HASHTABLE_H
#define __HASHTABLE_H

struct HashNode {
	void* Pair;
	char* Key;
	struct HashNode* Next;
};

struct HashTable {
	struct HashNode** Table;
	int TblSize;
	int Size;
};

int Hash_Find(struct HashTable* _Hash, const char* _Key, void** _Pair);
void Hash_Insert(struct HashTable* _Hash, const char* _Key, void* _Pair);

#endif /* HASHTABLE_H */
