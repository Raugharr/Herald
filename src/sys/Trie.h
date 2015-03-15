#ifndef __TRIE_H
#define __TRIE_H

struct LinkedList;

struct TrieNode {
	void* Data;
	struct TrieNode** Children;
	int ChildrenCt;
};

struct Trie {
	struct TrieNode* Root;
	int Size;
};

struct Trie* CreateTrie();
void DestroyTrie(struct Trie* _Trie);

void TrieInsert(struct Trie* _Trie, const char* _Str, void* _Data);
void* TrieSearch(struct Trie* _Trie, const char* _Str);
void TrieParSearch(struct Trie* _Trie, struct LinkedList* _List, const char* _Str);
void TrieDelete(struct Trie* _Trie, const char* _Str);
#endif
