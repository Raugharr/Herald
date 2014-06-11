/*
 * File: RBTree.c
 * Author: David Brotz
 */

#include "RBTree.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define RB_RED (0)
#define RB_BLACK (1)
#define RB_DBLACK (2)
#define RB_STRNULL (4) //size of "NULL".
#define RB_CONTR (RB_STRPTR + RB_STRCLR + 2)
#define RB_STRCLR (2)
#define RB_STRPTR (8) //size of pointer in style of, FFFFFFFF.
#define RB_STRDELM (1)
#define RB_DELM " "

struct RBNode* __RBTree_Search(struct RBTree* _Tree, void* _Data) {
	struct RBNode* _Node = NULL;
	int _Cmp = 0;

	if(_Data == NULL)
		return NULL;
	_Node = _Tree->Table;
	while(_Node != NULL) {
		_Cmp = _Tree->SCallback(_Data, _Node->Data);
		if(_Cmp == 0)
			return _Node;
		else if(_Cmp < 0)
			_Node = _Node->Left;
		else
			_Node = _Node->Right;
	}
		return NULL;
}

struct RBNode* __RBMax(struct RBNode* _Node) {
	if(_Node == NULL)
		return NULL;
	if(_Node->Left == NULL)
		return _Node;
	_Node = _Node->Left;
	while(_Node->Right != NULL)
		_Node = _Node->Right;
	return _Node;
}
struct RBNode* __RBMin(struct RBNode* _Node) {
	if(_Node == NULL)
		return NULL;
	if(_Node->Right == NULL)
		return _Node;
	_Node = _Node->Right;
	while(_Node->Left != NULL)
		_Node = _Node->Left;
	return _Node;
}

struct RBNode* RBSibling(struct RBNode* _Node) {
	if(_Node->Parent == NULL)
		return NULL;
	if(_Node == _Node->Parent->Left)
		return _Node->Parent->Right;
	return _Node->Parent->Left;
}

void RBReplace(struct RBNode* _Node, struct RBNode* _Rep) {
	_Rep->Parent = _Node->Parent;
	_Rep->Left = _Node->Left;
	_Rep->Right = _Node->Right;
}

void RBRotateLeft(struct RBTree* _Tree, struct RBNode* _Pivot) {
	struct RBNode* _Parent = _Pivot->Parent;
	struct RBNode* _Node = _Pivot->Right;

	if(_Node == NULL)
			return;
	_Pivot->Right = _Node->Left;
	if(_Node->Right != NULL) {
		if(_Node->Left != NULL)
			_Node->Left->Parent = _Pivot;
		else
			_Node->Left = NULL;
	}
	_Node->Parent = _Pivot->Parent;
	if(_Pivot->Parent == NULL)
		_Tree->Table = _Node;
	else
		if(_Pivot == _Parent->Left)
			_Parent->Left = _Node;
		else
			_Parent->Right = _Node;
	_Node->Left = _Pivot;
	_Pivot->Parent = _Node;
}

void RBRotateRight(struct RBTree* _Tree, struct RBNode* _Pivot) {
	struct RBNode* _Parent = _Pivot->Parent;
	struct RBNode* _Node = _Pivot->Right;

	if(_Node == NULL)
		return;
	_Pivot->Left = _Node->Right;
	if(_Node->Left != NULL) {
		if(_Node->Right != NULL)
			_Node->Right->Parent = _Pivot;
		else
			_Node->Right = NULL;
	}
	_Node->Parent = _Pivot->Parent;
	if(_Pivot->Parent == NULL)
		_Tree->Table = _Node;
	else
		if(_Pivot == _Parent->Right)
			_Parent->Right = _Node;
		else
			_Parent->Left = _Node;
	_Node->Right = _Pivot;
	_Pivot->Parent = _Node;
}

struct RBNode* RBGrandparent(struct RBNode* _Node) {
	if(_Node != NULL && _Node->Parent != NULL)
		return _Node->Parent->Parent;
	return NULL;
}

struct RBNode* RBUncle(struct RBNode* _Node) {
	struct RBNode* _Grand = RBGrandparent(_Node);

	if(_Grand == NULL)
		return _Grand;
	if(_Node->Parent == _Grand->Left)
		return _Grand->Right;
	return _Grand->Left;
}

struct RBNode* CreateRBNode(struct RBNode* _Parent, void* _Data, int _Color) {
	struct RBNode* _Node = (struct RBNode*) malloc(sizeof(struct RBNode));

	_Node->Color = _Color;
	_Node->Data = _Data;
	_Node->Parent = _Parent;
	_Node->Left = NULL;
	_Node->Right = NULL;
	return _Node;
}

struct RBTree* CreateRBTree(int(*_ICallBack)(void*, void*), int(*_SCallBack)(void*, void*)) {
	struct RBTree* _Tree = (struct RBTree*) malloc(sizeof(struct RBTree));

	_Tree->Table = NULL;
	_Tree->Size = 0;
	_Tree->ICallback = _ICallBack;
	_Tree->SCallback = _SCallBack;
	return _Tree;
}

void DestroyRBTree(struct RBTree* _Tree) {
	free(_Tree);
}

void RBTree_Balance(struct RBTree* _Tree, struct RBNode* _Node) {
	struct RBNode* _Parent = NULL;
	struct RBNode* _Uncle = NULL;

	while(_Node != _Tree->Table && _Node->Parent->Color == RB_RED) {
		_Parent = _Node->Parent;
		if(_Parent->Parent == NULL)
			break;
		if(_Parent == _Parent->Parent->Left) {
			_Uncle = _Parent->Parent->Right;
			if(_Uncle != NULL && _Uncle->Color == RB_RED) {
				_Parent->Color = RB_BLACK;
				_Uncle->Color = RB_BLACK;
				_Parent->Parent->Color = RB_RED;
				_Node = _Parent->Parent;
			} else {
				if(_Node == _Parent->Right) {
					_Node = _Parent;
					RBRotateLeft(_Tree, _Node);
				}
				_Parent->Color = RB_BLACK;
				_Parent->Parent->Color = RB_RED;
				RBRotateRight(_Tree, _Parent->Parent);
			}
		} else if(_Parent == _Parent->Parent->Right) {
			_Uncle = _Parent->Parent->Left;
			if(_Uncle != NULL && _Uncle->Color == RB_RED) {
				_Parent->Color = RB_BLACK;
				_Uncle->Color = RB_BLACK;
				_Parent->Parent->Color = RB_RED;
				_Node = _Parent->Parent;
			} else {
				if(_Node == _Parent->Left) {
					_Node = _Parent;
					RBRotateRight(_Tree, _Node);
				}
				_Parent->Color = RB_BLACK;
				_Parent->Parent->Color = RB_RED;
				RBRotateLeft(_Tree, _Parent->Parent);
			}
		}
	}
	_Tree->Table->Color = RB_BLACK;
}

void RBInsert(struct RBTree* _Tree, void* _Data) {
	struct RBNode* _Itr = NULL;
	struct RBNode* _Node = NULL;

	if(_Data == NULL)
		return;
#ifdef DEBUG
	assert(RBSearch(_Tree, _Data) == NULL);
#endif
	if(_Tree->Size == 0) {
		_Tree->Table = CreateRBNode(NULL, _Data, RB_BLACK);
		++_Tree->Size;
		return;
	}

	_Itr = _Tree->Table;

	while(_Itr != NULL) {
		if(_Tree->ICallback(_Data, _Itr->Data) < 0)
			if(_Itr->Left == NULL) {
				_Node = CreateRBNode(_Itr, _Data, RB_RED);
				_Itr->Left = _Node;
				break;
			} else
			_Itr = _Itr->Left;
		else
			if(_Itr->Right == NULL) {
				_Node = CreateRBNode(_Itr, _Data, RB_RED);
				_Itr->Right = _Node;
				break;
			} else
			_Itr = _Itr->Right;
	}
	RBTree_Balance(_Tree, _Node);
	++_Tree->Size;
}

void* RBSearch(struct RBTree* _Tree, void* _Data) {
	struct RBNode* _Node = __RBTree_Search(_Tree, _Data);

	if(_Node != NULL)
		return _Node->Data;
	return NULL;
}

struct RBNode* __RBDeleteRoot(struct RBTree* _Tree) {
	return _Tree->Table;
}

void RBDeleteRoot(struct RBTree* _Tree) {
	__RBDeleteRoot(_Tree);
}

void RBDelete(struct RBTree* _Tree, void* _Data) {
	struct RBNode* _Node = __RBTree_Search(_Tree, _Data);
	struct RBNode* _OldNode = NULL;
	struct RBNode* _Parent = NULL;
	struct RBNode* _Sibling = NULL;

	if(_Node == NULL)
		return;

	if(_Node->Right != NULL)
		_OldNode = __RBMin(_Node);
	else if(_Node->Left != NULL)
		_OldNode = __RBMax(_Node);

	if(_OldNode == NULL)
		_OldNode = _Node;
	_Node->Data = _OldNode->Data;
	_Parent = _OldNode->Parent;
	if(_Parent) {
		if(_Parent->Right == _OldNode) {
			_Parent->Right = _OldNode->Left;
			if(_OldNode->Left)
				_OldNode->Left->Parent = _Parent;
		} else {
			_Parent->Left = _OldNode->Left;
			if(_OldNode->Left)
				_OldNode->Left->Parent = _Parent;
		}
	} else {
#ifdef DEBUG
		assert(_Tree->Table->Left != NULL || _Tree->Table->Right != NULL);
#endif
		free(_Node);
		_Tree->Table = NULL;
		_Tree->Size = 0;
		return;
	}
	if(_OldNode->Color == RB_BLACK) {
		if(_Node->Color == RB_RED)
			_Node->Color = RB_BLACK;
		else
			_Node->Color = RB_DBLACK;
	}
	while(_Tree->Table != _Node && _Node->Color == RB_DBLACK) {
		_Sibling = RBSibling(_Node);
		if(_Sibling != NULL && _Sibling->Color == RB_BLACK) {
			if(_Sibling->Left != NULL && _Sibling->Left->Color == RB_RED) {
				RBRotateRight(_Tree, _Sibling);
				RBRotateLeft(_Tree, _Sibling->Parent); //_Sibling->Parent was _Sibling->Left before the above rotate.
				_Node->Color = RB_BLACK;
			} else if(_Sibling->Right != NULL && _Sibling->Right->Color == RB_RED) {
				_Sibling->Right->Color = RB_BLACK;
				RBRotateLeft(_Tree, _Sibling);
				_Node->Color = RB_BLACK;
			}
		} else if(_Sibling != NULL && _Sibling->Color == RB_RED) {
			_Sibling->Color = RB_BLACK;
			_Parent->Color = RB_RED;
			if(_Parent->Right == _Sibling)
				RBRotateRight(_Tree, _Sibling);
			else
				RBRotateLeft(_Tree, _Sibling);
			continue;
		}
		if((_Parent = _Node->Parent) != NULL) {
			if(_Parent->Color == RB_RED) {
				_Node->Color = RB_BLACK;
				_Parent->Color = RB_BLACK;
				if(_Sibling != NULL) {
					_Sibling->Color = RB_RED;
				} else {
					_Parent->Color = RB_DBLACK;
					_Node = _Parent;
				}
			} else {
				_Node->Color = RB_BLACK;
				_Parent->Color = RB_DBLACK;
				if(_Sibling != NULL)
					_Sibling->Color = RB_RED;
				_Node = _Parent;
				continue;
			}
		}
	}
	_Tree->Table->Color = RB_BLACK;
	--_Tree->Size;
	free(_OldNode);
}

struct RBItrStack* CreateRBStackNode(struct RBNode* _Node, struct RBItrStack* _Prev) {
	struct RBItrStack* _Itr = (struct RBItrStack*) malloc(sizeof(struct RBItrStack));

	_Itr->Node = _Node;
	_Itr->Prev = _Prev;
	return _Itr;
}

void RBIterate(struct RBNode* _Node, void(*_Callback)(void*)) {
	struct RBItrStack* _Stack = NULL;
	struct RBItrStack* _Delete = NULL;
	struct RBNode* _Itr = NULL;

	_Stack = CreateRBStackNode(NULL, NULL);
	_Itr = _Node;

	while(_Itr != NULL) {
		_Callback(_Itr->Data);
		if(_Itr->Right != NULL) {
			_Stack = CreateRBStackNode(_Itr->Right, _Stack);
		}
		if(_Itr->Left != NULL) {
			_Stack = CreateRBStackNode(_Itr->Left, _Stack);
		}
		_Delete = _Stack;
		_Itr = _Stack->Node;
		_Stack = _Stack->Prev;
		free(_Delete);
	}
}

void* RBMax(struct RBNode* _Node) {
	return __RBMax(_Node)->Data;
}
void* RBMin(struct RBNode* _Node) {
	return __RBMin(_Node)->Data;
}

int RBHeight_Aux(struct RBNode* _Node) {
	if(_Node == NULL)
		return 0;
	return RBHeight_Aux(_Node->Left) + RBHeight_Aux(_Node->Right) + 1;
}

int RBHeight(struct RBNode* _Node) {
	int _Left = 0;
	int _Right = 0;

	if(_Node == NULL)
		return 0;

	_Left = RBHeight_Aux(_Node->Left);
	_Right = RBHeight_Aux(_Node->Right);
	return (_Left < _Right) ? (_Right) : (_Left);
}

int RBColorCheck_Aux(struct RBNode* _Node) {
	if(_Node == NULL)
		return 1;
	return RBColorCheck_Aux(_Node->Left) == RBColorCheck_Aux(_Node->Right);
}

int RBColorCheck(struct RBNode* _Node) {
	if(_Node == NULL)
		return 1;
	return RBColorCheck(_Node->Left) == RBColorCheck(_Node->Right);
}

int RBStrlen(struct RBNode* _Node) {
	if(_Node == NULL)
		return RB_STRNULL + RB_STRDELM;
	return RBStrlen(_Node->Left) + RBStrlen(_Node->Right) + RB_CONTR + RB_STRDELM;
}

int RBToString(struct RBNode* _Node, char* _Buffer, int _Size) {
	if(_Node == NULL) {
		if(_Size < RB_STRNULL + RB_STRDELM + 1)
			return 0;
		strncat(_Buffer, "NULL ", _Size);
		_Buffer += RB_STRNULL;
		return RB_STRNULL;
	}
	if(_Size < RB_CONTR + 1)
		return 0;
	char _Temp[RB_CONTR + 1];
	sprintf(_Temp, "[%p %d]", _Node, _Node->Color);
	strncat(_Buffer, _Temp, _Size);
	strcat(_Buffer, RB_DELM);
	_Buffer += RB_CONTR + RB_STRDELM;
	_Size -= RB_CONTR + RB_STRDELM;
	return RBToString(_Node->Left, _Buffer, _Size) + RBToString(_Node->Right, _Buffer, _Size);
}
