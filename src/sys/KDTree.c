/*
 * File: KDTree.c
 * Author: David Brotz
 */

#include "KDTree.h"

#include "Array.h"
#include "LinkedList.h"

#include <stdlib.h>

#define KDX (0)
#define KDY (1)

int KDXCmp(const void* _One, const void* _Two) {
	return ((struct KDNode*)_One)->Pos[KDX] - ((struct KDNode*)_Two)->Pos[KDX];
}

int KDYCmp(const void* _One, const void* _Two) {
	return ((struct KDNode*)_One)->Pos[KDY] - ((struct KDNode*)_Two)->Pos[KDY];
}

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
	++_Tree->Size;
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

struct KDNode* KDBalance_Aux(struct KDNode** _Array, int _Axis, int _Size) {
	struct KDNode* _Node = NULL;
	int _Pos = 0;
	int _RightSize = 0;

	if(_Size <= 0)
		return NULL;
	if(_Size == 1) {
		_Array[0]->Left = NULL;
		_Array[0]->Right = NULL;
		return _Array[0];
	}
	_Axis = (_Axis & 1);
	QuickSort(_Array, _Size, (((_Axis & KDY) == KDY) ? (KDYCmp) : (KDXCmp)));
	_Pos = _Size / 2;

	_RightSize = _Size - (_Pos + 1);
	_Node = _Array[_Pos];
	_Node->Left = NULL;
	_Node->Right = NULL;
	if(_Pos != 0) {
		_Node->Left = KDBalance_Aux(_Array, _Axis + 1, _Pos);
		_Node->Left->Parent = _Node;
	}
	if(_Pos + 1 < _Size) {
		_Node->Right = KDBalance_Aux(_Array + (_Pos + 1), _Axis + 1, _RightSize);
		_Node->Right->Parent = _Node;
	}
	return _Node;
}

void KDBalance(struct KDTree* _Tree) {
	struct KDNode* _Array[_Tree->Size];

	KDToArray(_Array, _Tree->Root, KDX, _Tree->Size);
	_Tree->Root = KDBalance_Aux(_Array, KDX, _Tree->Size);
	_Tree->Root->Parent = NULL;
	free(_Array);
}

const struct KDNode** KDToArray_Aux(const struct KDNode* _Node, int _Axis, int* _Count, int _Size, const struct KDNode** _Array) {
	const struct KDNode** _Left = NULL;

	if(_Node == NULL)
		return NULL;
	_Array[(*_Count)++] = _Node;
	if((_Left = KDToArray_Aux(_Node->Left, _Axis, _Count, _Size, _Array)) == NULL) {
		KDToArray_Aux(_Node->Right, _Axis, _Count, _Size, _Array);
	} else
		KDToArray_Aux(_Node->Right, _Axis, _Count, _Size, _Array);
	return _Array;
}

struct KDNode** KDToArray(struct KDNode** _List, const struct KDNode* _Node, int _Axis, int _Size) {
	int _Count = 0;

	KDToArray_Aux(_Node, _Axis, &_Count, _Size, (const struct KDNode**)_List);
	return _List;
}

struct LinkedList* KDRange_Aux(struct KDNode* _Node, int _Axis, int _Pos[2], int _Area[2], struct LinkedList* _List) {
	int _End[2];

	if(_Pos[KDX] < 0 || _Pos[KDY] < 0 || _Area[KDX] < 0 || _Area[KDY] < 0 || _Node == NULL)
		return NULL;
	if(_List == NULL)
		_List = CreateLinkedList();
	_Axis = ((_Axis & KDY) == KDY) ? (KDY) : (KDX);
	_End[KDX] = _Pos[KDX] + _Area[KDX];
	_End[KDY] = _Pos[KDY] + _Area[KDY];
	if(_Node->Pos[KDX] >= _Pos[KDX] && _Node->Pos[KDX] <= _End[KDX]
		&& _Node->Pos[KDY] >= _Pos[KDY] && _Node->Pos[KDY] <= _End[KDY])
		LnkLst_PushBack(_List, _Node->Data);
	if(_Node->Right)
		if(_Node->Right->Pos[_Axis] < _Pos[_Axis])
			goto right;
	KDRange_Aux(_Node->Left, _Axis + 1, _Pos, _Area, _List);
	right:
	KDRange_Aux(_Node->Right, _Axis + 1, _Pos, _Area, _List);
	return _List;
}

int KDHeightNode(struct KDNode* _Node) {
	int _Left = 0;
	int _Right = 0;

	if(_Node == NULL)
		return 0;
	_Left = KDHeightNode(_Node->Left);
	_Right = KDHeightNode(_Node->Right);
	return ((_Left > _Right) ? (_Left) : (_Right)) + 1;
}

struct KDNode* KDNextNode(struct KDNode* _Node) {
	if(_Node->Left != NULL)
		return _Node->Left;
	else
		right:
		if(_Node->Right != NULL)
			return _Node->Right;
	else
		while(_Node->Parent != NULL) {
			if(_Node == _Node->Parent->Left) {
				_Node = _Node->Parent;
				goto right;
			}
			_Node = _Node->Parent;
		}
	return NULL;
}
