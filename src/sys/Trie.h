#ifndef __TRIE_H
#define __TRIE_H

struct TrieNode {
	char Character;
	void* Data;
	struct TrieNode** Children;
	int ChildrenSz;
};

struct Trie {
	struct TrieNode* Root;
	int Size;
};

struct Trie* CreateTrie();
void DestroyTrie(struct Trie* _Trie);

void TrieInsert(struct Trie* _Trie, const char* _Str, void* _Data);
void* TrieSearch(struct Trie* _Trie, const char* _Str);

#endif
