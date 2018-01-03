#include "Trie.h"

#include "Array.h"
#include "LinkedList.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define TRIE_ALPHEBET (26)
#define TRIE_OFFSET ('a')

struct TrieNode* CreateTrieNode() {
	struct TrieNode* _Node = (struct TrieNode*) malloc(sizeof(struct TrieNode));

	_Node->Data = NULL;
	_Node->Children = calloc(sizeof(struct TrieNide*), TRIE_ALPHEBET);
	return _Node;
}

void DestroyTrieNode(struct TrieNode* _Node) {
	int i = 0;

	for(i = 0; i < TRIE_ALPHEBET; ++i) {
		if(_Node->Children[i] != NULL) {
			DestroyTrieNode(_Node->Children[i]);
			_Node->Children[i] = NULL;
		}
	}
}

struct Trie* CreateTrie() {
	struct Trie* _Trie = (struct Trie*) malloc(sizeof(struct Trie));
	struct TrieNode* _Node = (struct TrieNode*) malloc(sizeof(struct TrieNode));

	_Node->Data = NULL;
	_Node->Children = malloc(TRIE_ALPHEBET * sizeof(struct TrieNode*));
	_Trie->Root = _Node;
	_Trie->Size = 0;
	return _Trie;
}

void DestroyTrie(struct Trie* _Trie) {
	DestroyTrieNode(_Trie->Root);
	free(_Trie);
}

void TrieInsert(struct Trie* _Trie, const char* _Str, void* _Data) {
	struct TrieNode* _Node = _Trie->Root;
	int _Offset = 0;;
	char _Char = '\0';

	while(*_Str != '\0') {
		_Char = tolower(*_Str);
		_Offset = _Char - TRIE_OFFSET;
		if(_Node->Children[_Offset] == NULL)
			_Node->Children[_Offset] = CreateTrieNode();
		++_Node->ChildrenCt;
		_Node = _Node->Children[_Offset];
		++_Str;
	}
	_Node->Data = _Data;
}

struct TrieNode* TrieSearchNode(struct Trie* _Trie, const char* _Str) {
	struct TrieNode* _Node = _Trie->Root;
	int _Offset = 0;
	char _Char = '\0';

	while(*_Str != '\0') {
		_Char = tolower(*_Str);
		_Offset = _Char - TRIE_OFFSET;
		if(_Node->Children[_Offset] == NULL)
			return NULL;
		_Node = _Node->Children[_Offset];
		++_Str;
	}
	if(*_Str == '\0')
		return _Node;
	return NULL;
}

void* TrieSearch(struct Trie* _Trie, const char* _Str) {
	struct TrieNode* _Node = TrieSearchNode(_Trie, _Str);

	if(_Node != NULL)
		return _Node->Data;
	return _Node;
}

void TrieParSearch_Aux(struct Trie* _Trie, struct LinkedList* _List, struct TrieNode* _Node) {
	int i = 0;

	if(_Node->Data != NULL)
		LnkLstPushBack(_List, _Node->Data);
	for(i = 0; i < TRIE_ALPHEBET; ++i) {
		if(_Node->Children[i] != NULL)
			TrieParSearch_Aux(_Trie, _List, _Node->Children[i]);
	}
}

void TrieParSearch(struct Trie* _Trie, struct LinkedList* _List, const char* _Str) {
	struct TrieNode* _Node = TrieSearchNode(_Trie, _Str);

	TrieParSearch_Aux(_Trie, _List, _Node);
}

int TrieDelete_Aux(struct TrieNode* _Node, const char* _Str) {
	int _Offset = 0;

	if(_Node == NULL)
		return 0;
	if(*_Str == '\0')
		return 1;
	_Offset = tolower(*_Str) - TRIE_OFFSET;	
	if(TrieDelete_Aux(_Node->Children[_Offset], _Str + 1) == 1) {
		free(_Node->Children[_Offset]);
		_Node->Children[_Offset] = NULL;
		if(_Node->ChildrenCt <= 1)
			return 1;
	}
	--_Node->ChildrenCt;
	return 0;
}

void TrieDelete(struct Trie* _Trie, const char* _Str) {
	TrieDelete_Aux(_Trie->Root, _Str);	
}
