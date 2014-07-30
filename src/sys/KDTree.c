/*
 * File: KDTree.c
 * Author: David Brotz
 */

#include "KDTree.h"

#include <stdlib.h>

#define KDX (0)
#define KDY (1)

struct KDNode* CreateKDNode(void* _Data, int _X, int _Y) {
	struct KDNode* _Node = (struct KDNode*) malloc(sizeof(struct KDNode));

	_Node->Parent = NULL;
	_Node->Left = NULL;
	_Node->Right = NULL;
	_Node->Pos[KDX] = _X;
	_Node->Pos[KDY] = _Y;
	_Node->Data = _Data;
	return _Node;
}

struct KDTree* CreateKDTree() {
	struct KDTree* _Tree = (struct KDTree*) malloc(sizeof(struct KDTree));

	_Tree->Root = NULL;
	_Tree->Size = 0;
	return _Tree;
}

void DestroyKDTree(struct KDTree* _Tree) {
	KDDeleteAll(_Tree);
	free(_Tree);
}

void KDDeleteAll_Aux(struct KDNode* _Node) {
	if(_Node == NULL)
		return;
	KDDeleteAll_Aux(_Node->Left);
	KDDeleteAll_Aux(_Node->Right);
	free(_Node);
}

void KDDeleteAll(struct KDTree* _Tree) {
	KDDeleteAll_Aux(_Tree->Root);
	_Tree->Root = NULL;
	_Tree->Size = 0;
}

void* KDInsertSearch(struct KDTree* _Tree, void* _Data, int _Pos[2]) {
	struct KDNode* _Search = _Tree->Root;
	struct KDNode* _Node = NULL;
	int _Axis = KDX;

	if(_Search == NULL) {
		_Tree->Root = CreateKDNode(_Data, _Pos[KDX], _Pos[KDY]);
		_Tree->Size = 1;
		return NULL;
	}

	do {
		if(_Pos[_Axis] < _Search->Pos[_Axis]) {
			if(_Search->Left == NULL) {
				_Node = CreateKDNode(_Data, _Pos[KDX], _Pos[KDY]);
				_Node->Parent = _Search->Left;
				_Search->Left = _Node;
				++_Tree->Size;
				return NULL;
			}
			_Search = _Search->Left;
		} else if(_Pos[_Axis] == _Search->Pos[_Axis]) {
			return _Search->Data;
		} else {
			if(_Search->Right == NULL) {
				_Node = CreateKDNode(_Data, _Pos[KDX], _Pos[KDY]);
				_Node->Parent = _Search->Right;
				_Search->Right = _Node;
				++_Tree->Size;
				return NULL;
			}
			_Search = _Search->Right;
		}
		_Axis = (_Axis == KDX) ? (KDY) : (KDX);
	} while(_Search != NULL);
	return NULL;
}

void KDInsert(struct KDTree* _Tree, void* _Data, int _X, int _Y) {
	struct KDNode* _Node = CreateKDNode(_Data, _X, _Y);
	struct KDNode* _Parent = _Tree->Root;
	int _Axis = KDX;

	if(_Tree->Root == NULL) {
		_Tree->Root = _Node;
		_Tree->Size = 1;
		return;
	}

	while(1) {
		if(_Node->Pos[_Axis] < _Parent->Pos[_Axis]) {
			if(_Parent->Left == NULL) {
				_Node->Parent = _Parent;
				_Parent->Left = _Node;
				break;
			}
			_Parent = _Parent->Left;
		} else {
			if(_Parent->Right == NULL) {
				_Node->Parent = _Parent;
				_Parent->Right = _Node;
				break;
			}
			_Parent = _Parent->Right;
		}
		_Axis = (_Axis == KDX) ? (KDY) : (KDX);
	}
}

struct KDNode* KDSearchNode(const struct KDTree* _Tree, int _Data[2]) {
	struct KDNode* _Node = _Tree->Root;
	int _Axis = KDX;

	while(_Node != NULL) {
		if(_Data[_Axis] < _Node->Pos[_Axis]) {
			_Node = _Node->Left;
		} else if(_Data[_Axis] == _Node->Pos[_Axis])
			return _Node;
		else {
			_Node = _Node->Right;
		}
		_Axis = (_Axis == KDX) ? (KDY) : (KDX);
	}
	return NULL;
}

void* KDSearch(const struct KDTree* _Tree, int _Data[2]) {
	struct KDNode* _Node = KDSearchNode(_Tree, _Data);

	if(_Node != NULL)
		return _Node->Data;
	return NULL;
}
