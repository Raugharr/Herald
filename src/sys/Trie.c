#include "Trie.h"

#include "Array.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define TRIE_ALPHEBET (26)
#define TRIE_OFFSET (91)

struct Trie* CreateTrie() {
	struct Trie* _Trie = (struct Trie*) malloc(sizeof(struct Trie));
	struct TrieNode* _Node = (struct TrieNode*) malloc(sizeof(struct TrieNode));

	_Node->Data = NULL;
	_Node->Children = malloc(TRIE_ALPHEBET * sizeof(struct TrieNode*));
	_Node->ChildrenSz = TRIE_ALPHEBET;
	_Trie->Root = _Node;
	_Trie->Size = 0;
	return _Trie;
}

void DestroyTrie(struct Trie* _Trie) {
	free(_Trie);
}

struct TrieNode* CreateTrieNode(char _Char) {
	struct TrieNode* _Node = (struct TrieNode*) malloc(sizeof(struct TrieNode));

	_Node->Character = _Char;
	_Node->Data = NULL;
	_Node->ChildrenSz = 8;
	_Node->Children = calloc(sizeof(struct TrieNide*), _Node->ChildrenSz);
	return _Node;
}

void TrieInsert(struct Trie* _Trie, const char* _Str, void* _Data) {
	struct TrieNode* _Node = _Trie->Root;
	int j = 0;
	char _Char = '\0';

	while(*_Str != '\0') {
		_Char = tolower(*_Str);
		for(j = 0; j < _Node->ChildrenSz; ++j) {
			if(_Node->Children[j] == NULL) {
				_Node->Children[j] = CreateTrieNode(_Char);
				_Node = _Node->Children[j];
				goto end_loop;
			}
			if(_Node->Children[j]->Character == _Char) {
				_Node = _Node->Children[j];
				goto end_loop;
			}
		}
		if(_Node->ChildrenSz < TRIE_ALPHEBET / 2) {
			_Node->ChildrenSz = _Node->ChildrenSz * 2;
			_Node->Children = realloc(_Node->Children, _Node->ChildrenSz * sizeof(struct TrieNode*));	
		} else {
			_Node->ChildrenSz = TRIE_ALPHEBET;
			_Node->Children = realloc(_Node->Children, _Node->ChildrenSz * sizeof(struct TrieNode*));
		}
		_Node->Children[j] = CreateTrieNode(_Char);
		_Node = _Node->Children[j];
		end_loop:
		++_Str;
	}
	_Node->Data = _Data;
}
void* TrieSearch(struct Trie* _Trie, const char* _Str) {
	struct TrieNode* _Node = _Trie->Root;
	int i = 0;

	while(*_Str != '\0') {
		for(i = 0; i < _Node->ChildrenSz; ++i) {
			if(_Node->Children[i] == NULL)
				return NULL;
			if(_Node->Children[i]->Character == tolower(*_Str)) {
				_Node = _Node->Children[i];
				break;
			}
		}
		++_Str;
	}
	if(*_Str == '\0')
		return _Node->Data;
	return NULL;
}
