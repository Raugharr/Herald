/**
 * Author: David Brotz
 * IntervalTree.c
 */

#include "IntervalTree.h"

#include "Array.h"
#include "Math.h"
#include "LinkedList.h"

#include <limits.h>
#include <string.h>
#include <stdlib.h>

#define ITreeLeft(_Idx) ((_Idx) * 2 + 1)
#define ITreeRight(_Idx) ((_Idx) * 2 + 2)
#define ITreeParent(_Idx) (((_Idx) - 1) / 2)
#define ITreeRootNode(_Tree) (_Tree->Table[0])

static inline int ITreeSpan(int _NodeMin, int _NodeMax, int _Min, int _Max) {
	return (_NodeMin >= _Min && _NodeMax <= _Max);
	//return (_Min <= _NodeMin && _Max <= _NodeMax);
}

int ITreePointCmp(const int* _One, const int _Two) {
	return *_One - _Two;
}

int TempCmp(const int* _One, const int* _Two) {
	return *_One - *_Two;
}

void ITreeSetValues(struct IntervalTree* _Tree, int* _Points, int _Idx, int _Min, int _Max) {
	int _Mid = 0;

	if(_Min >= _Max)
		return;
	_Mid = (_Min + _Max) / 2;
	_Tree->Table[_Idx].Split = _Points[_Mid];
	ITreeSetValues(_Tree, _Points, ITreeLeft(_Idx), _Min, _Mid);
	ITreeSetValues(_Tree, _Points, ITreeRight(_Idx), _Mid + 1, _Max);
}

void ITreeAddData(struct IntervalTree* _Tree, int  _NodeIdx, int _Min, int _Max, void* _Data, int _DMin, int _DMax) {
	struct ITreeNode* _Node = NULL;

	if(_NodeIdx >= _Tree->TableSz)
		return;
	_Node = &_Tree->Table[_NodeIdx];

	if(ITreeSpan(_Min, _Max, _DMin, _DMax) != 0) {
		if(_Node->Table == NULL) {
			_Node->TableSz = 1;
			_Node->Table = calloc(_Node->TableSz, sizeof(struct ITreeNode*));
			_Node->Table[0] = _Data;
		} else {
			/*for(int i = 0; i < _Node->TableSz; ++i) {
				if(_Node->Table[i] == NULL) {
					_Node->Table[i] = _Data;
					goto exit_loop;
				}
			}*/
			_Node->TableSz += 1;
			_Node->Table = realloc(_Node->Table, _Node->TableSz * sizeof(struct ITreeNode*));
			_Node->Table[_Node->TableSz - 1] = _Data;
		}
		return;
	}
	if(_DMin < _Node->Split)
		ITreeAddData(_Tree, ITreeLeft(_NodeIdx), _Min, _Node->Split, _Data, _DMin, _DMax);
	if(_DMax > _Node->Split)
		ITreeAddData(_Tree, ITreeRight(_NodeIdx), _Node->Split, _Max, _Data, _DMin, _DMax);
}

const struct IntervalTree* ConstructITree(void** _Values, int _ValueSz, ITreeGetVal _MinFunc, ITreeGetVal _MaxFunc) {
	struct IntervalTree* _ITree = malloc(sizeof(struct IntervalTree));
	int* _Points = calloc(_ValueSz * 2, sizeof(int));
	int _Size = 0;
	int _Val = 0;
	//Count of how many leafs are required to give every leaf two children.
	int _Leafs = 0;
	int _Height = 0;
	int _PointSz = 0;
	int _NodeIdx = 0;
	
	for(int i = 0; i < _ValueSz; ++i) {
		 _Val = _MinFunc(_Values[i]);
		 if(BinarySearch(&_Val, _Points, _Size, (CompCallback)ITreePointCmp) == 0) {
			_Points[_Size++] = _Val;
		 }
		_Val = _MaxFunc(_Values[i]);
		 if(BinarySearch(&_Val, _Points, _Size, (CompCallback)ITreePointCmp) == 0) {
			_Points[_Size++] = _Val;
		 }
	}
	_PointSz = _Size;
	InsertionSort(_Points, _PointSz, (CompCallback) TempCmp, sizeof(int));
	_Height = NextPowTwo(_Size);
	_Leafs = (_Height - 1) - _Size; //Get number of leafs needed to complete the last row in the binary tree.
	_Leafs += (_Size - ((_Height >> 1) - 1)) * 2; //How many children the leafs in the last row of the binary tree have.
	_Size += _Leafs;
	_ITree->Table = calloc(_Size, sizeof(struct ITreeNode));
	_ITree->TableSz = _Size;
	ITreeSetValues(_ITree, _Points, 0, 0, _PointSz);
	//ITreeSetValues(_ITree, _Points, 0, _Points[0], _Points[_Size - 1]);
	for(int i = 0; i < _ValueSz; ++i) {
		//NOTE: Can we remove _MinFunc and _MaxFuncs by storing their results from the above loop?
		ITreeAddData(_ITree, 0, INT_MIN, INT_MAX, _Values[i], _MinFunc(_Values[i]), _MaxFunc(_Values[i]));
	}
	while(_NodeIdx < _ITree->TableSz) _NodeIdx = ITreeRight(_NodeIdx);
	_ITree->Table[ITreeParent(ITreeParent(_NodeIdx))].Split += 1;
	_NodeIdx = 0;
	while(_NodeIdx < _ITree->TableSz) _NodeIdx = ITreeLeft(_NodeIdx);
	_ITree->Table[ITreeParent(ITreeParent(_NodeIdx))].Split -= 1;
	free(_Points);
	return _ITree;
}

void DestroyITree(const struct IntervalTree* _Tree) {
	//FIXME: Don't forget to free the tables in each node.
	free((void*)_Tree->Table);
	free((void*)_Tree);
}

void ITreeQuery_Aux(const struct IntervalTree* _Tree, int _NodeIdx, int _Val, struct LinkedList* _List, int _Min, int _Max) {
	/*const struct ITreeNode* _Node = NULL;

	if(_NodeIdx >= _Tree->TableSz)
		return;
	_Node = &_Tree->Table[_NodeIdx];
	if(_Val >= _Min && _Val <= _Max) {
		for(int i = 0; i < _Node->TableSz; ++i) {
			if(_Node->Table[i] != NULL) {
				for(const struct LnkLst_Node* _Itr = _List->Front; _Itr != NULL; _Itr = _Itr->Next) {
					if(_Itr->Data == _Node->Table[i])
						goto escape_insert;
				}
				LnkLstPushBack(_List, _Node->Table[i]);
			}
			escape_insert:
			continue;
		}
	}
	if(_Val <= _Node->Split) {
		ITreeQuery_Aux(_Tree, ITreeLeft(_NodeIdx), _Val, _List, _Min, _Node->Split);
	}
	if(_Val >= _Node->Split) {
		ITreeQuery_Aux(_Tree, ITreeRight(_NodeIdx), _Val, _List, _Node->Split, _Max);
	}*/
	const struct ITreeNode* _Node = NULL;
	//int _NodeIdx = 0;

	if(_Tree->TableSz <= 0)
		return;
	do {
		_Node = &_Tree->Table[_NodeIdx];	
		for(int i = 0; i < _Node->TableSz; ++i)
			LnkLstPushBack(_List, _Node->Table[i]);
		if(_Val < _Node->Split)
			_NodeIdx = ITreeLeft(_NodeIdx);
		else
			_NodeIdx = ITreeRight(_NodeIdx);
	} while(_NodeIdx < _Tree->TableSz);
}
