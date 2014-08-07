/*
 * File: KDTree.c
 * Author: David Brotz
 */

#include "KDTree.h"

#include "Array.h"

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

struct KDNode* KDBalance_Aux(int _Axis, struct KDNode** _Array, int _Size, int _Median) {
	struct KDNode* _Node = NULL;
	int _Pos = 0;
	int _RightSize = 0;

	if(_Size <= 0)
		return NULL;
	if(_Size == 1)
		return _Array[0];
	InsertionSort(_Array, _Size, (((_Axis & 1) == KDX) ? (KDXCmp) : (KDYCmp)));
	_Pos = KDFindMedian(_Array, _Size, _Axis, _Median);

	_RightSize = _Size - (_Pos + 1);
	_Node = _Array[_Pos];
	_Node->Left = NULL;
	_Node->Right = NULL;
	_Node->Parent = NULL;
	_Node->Left = KDBalance_Aux(_Axis + 1, _Array, _Pos, KDArrayMedian((const struct KDNode** const)&_Array[_Pos - 1], _Pos - 1, ((_Axis + 1) & 1)));
	_Node->Left->Parent = _Node;
	if(_Pos + 1 < _Size) {
		_Node->Right = KDBalance_Aux(_Axis + 1, _Array + _Pos + 1, _RightSize, KDArrayMedian((const struct KDNode** const)&_Array[_Pos + 1], _RightSize, ((_Axis + 1) & 1)));
		_Node->Right->Parent = _Node;
	}
	return _Node;
}

void KDBalance(struct KDTree* _Tree) {
	int _Median = 0;
	struct KDNode** _Array = KDToArray(_Tree->Root, KDX, &_Median, _Tree->Size);

	_Tree->Root = KDBalance_Aux(KDX, _Array, _Tree->Size, _Median);
	free(_Array);
}

const struct KDNode** KDToArray_Aux(const struct KDNode* _Node, int _Axis, int* _Median, int* _Count, int _Size, const struct KDNode** _Array) {
	const struct KDNode** _Left = NULL;

	if(_Node == NULL)
		return NULL;
	_Array[(*_Count)++] = _Node;
	(*_Median) = (*_Median) + _Node->Pos[_Axis];
	if((_Left = KDToArray_Aux(_Node->Left, _Axis, _Median, _Count, _Size, _Array)) == NULL) {
		KDToArray_Aux(_Node->Right, _Axis, _Median, _Count, _Size, _Array);
	} else
		KDToArray_Aux(_Node->Right, _Axis, _Median, _Count, _Size, _Array);
	return _Array;
}


struct KDNode** KDToArray(const struct KDNode* _Node, int _Axis, int* _Median, int _Size) {
	int _Count = 0;
	struct KDNode** _List = (struct KDNode**) malloc(sizeof(struct KDNode*));
	KDToArray_Aux(_Node, _Axis, _Median, &_Count, _Size, (const struct KDNode**)_List);

	(*_Median) = (*_Median) / _Count;
	return _List;
}

int KDArrayMedian(const struct KDNode** const _Array, int _Size, int _Axis) {
	int i = 0;
	int _Count = 0;
	int _Median = 0;

	if(_Size <= 0)
		return 0;

	for(i = 0; i < _Size; ++i) {
		_Median += _Array[i]->Pos[_Axis];
		++_Count;
	}
	return _Median / _Count;
}

int KDFindMedian(struct KDNode** _Array, int _Size, int _Axis, int _Median) {
	int _Low = 0;
	int _High = _Size;
	int _Mid = 0;
	int _MidRes = 0;
	int _NextRes = 0;

	while(_High >= _Low) {
		_Mid = _Low + ((_High - _Low) / 2);

		_MidRes = _Array[_Mid]->Pos[_Axis] - _Median;
		_NextRes = _Array[_Mid - 1]->Pos[_Axis] - _Median;
		if(_MidRes >= 0 && _NextRes < 0)
			return _Mid;
		else if(_MidRes < 0)
			_Low = _Mid + 1;
		else {
			if(_NextRes < 0)
				_Low = _Mid - 1;
			else
				_High = _Mid - 1;
		}
	}
	return _Mid;
}
