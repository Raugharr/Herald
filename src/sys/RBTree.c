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
#define RBNodeSwap(_Copy, _Node)			\
	(_Copy)->Parent = (_Node)->Parent;		\
	(_Copy)->Left = (_Node)->Left;			\
	(_Copy)->Right = (_Node)->Right

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

void RBRotateLeft(struct RBTree* _Tree, struct RBNode* _Root) {
	struct RBNode* _Parent = _Root->Parent;
	struct RBNode* _Pivot = _Root->Right;

	if(_Pivot == NULL)
			return;
	_Root->Right = _Pivot->Left;
	if(_Pivot->Left != NULL)
		_Pivot->Left->Parent = _Root;
	_Pivot->Parent = _Root->Parent;
	if(_Root->Parent == NULL)
		_Tree->Table = _Pivot;
	else
		if(_Root == _Parent->Left)
			_Parent->Left = _Pivot;
		else
			_Parent->Right = _Pivot;
	_Pivot->Left = _Root;
	_Root->Parent = _Pivot;
}

void RBRotateRight(struct RBTree* _Tree, struct RBNode* _Root) {
	struct RBNode* _Parent = _Root->Parent;
	struct RBNode* _Pivot = _Root->Left;

	if(_Pivot == NULL)
		return;
	_Root->Left = _Pivot->Right;
	if(_Pivot->Right != NULL)
		_Pivot->Right->Parent = _Root;
	_Pivot->Parent = _Root->Parent;
	if(_Root->Parent == NULL)
		_Tree->Table = _Pivot;
	else
		if(_Root == _Parent->Right)
			_Parent->Right = _Pivot;
		else
			_Parent->Left = _Pivot;
	_Pivot->Right = _Root;
	_Root->Parent = _Pivot;
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

struct RBNode* CopyRBNode(struct RBNode* _Node) {
	struct RBNode* _NewNode = (struct RBNode*) malloc(sizeof(struct RBNode));

	_NewNode->Color = _Node->Color;
	_NewNode->Data = _Node->Data;
	_NewNode->Parent = NULL;
	if(_Node->Left) {
		_NewNode->Left = CopyRBNode(_Node->Left);
		_NewNode->Left->Parent = _NewNode;
	} else
		_NewNode->Left = NULL;

	if(_Node->Right) {
		_NewNode->Right = CopyRBNode(_Node->Right);
		_NewNode->Right->Parent = _NewNode;
	} else
		_NewNode->Right = NULL;

	return _NewNode;
}

struct RBTree* CreateRBTree(int(*_ICallBack)(const void*, const void*), int(*_SCallBack)(const void*, const void*)) {
	struct RBTree* _Tree = (struct RBTree*) malloc(sizeof(struct RBTree));

	_Tree->Table = NULL;
	_Tree->Size = 0;
	_Tree->ICallback = _ICallBack;
	_Tree->SCallback = _SCallBack;
	return _Tree;
}

struct RBTree* CopyRBTree(struct RBTree* _Tree) {
	struct RBTree* _NewTree = (struct RBTree*) malloc(sizeof(struct RBTree));

	if(_Tree->Table != NULL)
		_NewTree->Table = CopyRBNode(_Tree->Table);
	else
		_NewTree->Table = NULL;
	_NewTree->Size = _Tree->Size;
	_NewTree->ICallback = _Tree->ICallback;
	_NewTree->SCallback = _Tree->SCallback;
	return _NewTree;
}

void DestroyRBTree(struct RBTree* _Tree) {
	free(_Tree);
}

void RBBalance(struct RBTree* _Tree, struct RBNode* _Node) {
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
					_Parent = _Node->Parent;
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
					_Parent = _Node->Parent;
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
	RBBalance(_Tree, _Node);
	++_Tree->Size;
}

struct RBNode* RBInsertSearch(struct RBTree* _Tree, void* _Search, void* _Insert) {
	struct RBNode* _Itr = NULL;
	struct RBNode* _Node = NULL;
	int _Found;

	if(_Search == NULL || _Insert == NULL)
		return 0;
	if(_Tree->Size == 0) {
		_Tree->Table = CreateRBNode(NULL, _Insert, RB_BLACK);
		++_Tree->Size;
		return NULL;
	}
	_Itr = _Tree->Table;
	while(_Itr != NULL) {
		_Found = _Tree->ICallback(_Search, _Itr->Data);
		if(_Found < 0)
			if(_Itr->Left == NULL) {
				_Node = CreateRBNode(_Itr, _Insert, RB_RED);
				_Itr->Left = _Node;
				break;
			} else
			_Itr = _Itr->Left;
		else if(_Found > 0)
			if(_Itr->Right == NULL) {
				_Node = CreateRBNode(_Itr, _Insert, RB_RED);
				_Itr->Right = _Node;
				break;
			} else
				_Itr = _Itr->Right;
		else
			return _Itr;
	}
	RBBalance(_Tree, _Node);
	++_Tree->Size;
	return NULL;
}

void* RBSearch(const struct RBTree* _Tree, const void* _Data) {
	struct RBNode* _Node = RBSearchNode(_Tree, _Data);

	if(_Node != NULL)
		return _Node->Data;
	return NULL;
}

struct RBNode* RBSearchNode(const struct RBTree* _Tree, const void* _Data) {
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

void RBDelete(struct RBTree* _Tree, void* _Data) {
	RBDeleteNode(_Tree, RBSearchNode(_Tree, _Data));
}

/**
 * It is possible to just move the data field from _Node to _OldNode
 * but we want to be able to ensure that pointers to RBNode*s will not
 * be invalidated.
 */
void RBDeleteNode(struct RBTree* _Tree, struct RBNode* _OldNode) {
	struct RBNode* _Node = NULL;
	struct RBNode* _NewNode = NULL;
	struct RBNode* _Parent = NULL;
	struct RBNode* _Sibling = NULL;

	if(_OldNode == NULL)
		return;

	if(_Tree->Size == 1) {
		#ifdef DEBUG
				assert(_Tree->Table->Left == NULL && _Tree->Table->Right == NULL);
		#endif
		free(_OldNode);
		_Tree->Table = NULL;
		_Tree->Size = 0;
		return;
	}

	if(_OldNode->Right != NULL)
		_NewNode = __RBMin(_OldNode);
	else if(_OldNode->Left != NULL)
		_NewNode = __RBMax(_OldNode);
	else
		_NewNode = _OldNode;

	if(_NewNode == NULL)
		_Node = _OldNode;
	else
		_Node = _NewNode;

	if(_Node->Color == RB_RED)
		_Node->Color = RB_BLACK;
	else
		_Node->Color = RB_DBLACK;
	while(_Tree->Table != _Node && _Node->Color == RB_DBLACK) {
		_Sibling = RBSibling(_Node);
		_Parent = _Node->Parent;
		if(_Sibling != NULL && _Sibling->Color == RB_BLACK) {
			if(_Sibling->Left != NULL && _Sibling->Left->Color == RB_RED) {
				RBRotateRight(_Tree, _Sibling);
				RBRotateLeft(_Tree, _Sibling->Parent);
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
		if(_Node->Parent != NULL) {
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
	_Parent = _NewNode->Parent;
	if(_Parent) {
		struct RBNode* _Temp = (_NewNode->Left == NULL) ? (_NewNode->Right) : (_NewNode->Left);
		if(_Parent->Right == _NewNode)
			_Parent->Right = _Temp;
		else
			_Parent->Left = _Temp;
		if(_Temp)
			_Temp->Parent = _Parent;
	}

	if(_OldNode != _NewNode) {
		if(_OldNode->Parent != NULL) {
			if(_OldNode->Parent->Left == _OldNode)
				_OldNode->Parent->Left = _NewNode;
			else
				_OldNode->Parent->Right = _NewNode;
		}
		_NewNode->Parent = _OldNode->Parent;
		_NewNode->Left = _OldNode->Left;
		_NewNode->Right = _OldNode->Right;
		if(_NewNode->Right)
			_NewNode->Right->Parent = _NewNode;
		if(_NewNode->Left)
			_NewNode->Left->Parent = _NewNode;
		if(_OldNode == _Tree->Table)
			_Tree->Table = _NewNode;
	}
	_Tree->Table->Color = RB_BLACK;
	--_Tree->Size;
	free(_OldNode);
}

void RBDepthFirst_Aux(const struct RBNode* _Node, struct RBNode** _Stack, int _Index) {
	if(_Node == NULL)
		return;
	_Stack[_Index] = _Node;
	RBDepthFirst_Aux(_Node->Left, _Stack, _Index + 1);
	RBDepthFirst_Aux(_Node->Right, _Stack, _Index + 2);
}

struct RBItrStack* RBStackPush(struct RBItrStack* _Prev, struct RBNode* _Node) {
	struct RBItrStack* _Itr = (struct RBItrStack*) malloc(sizeof(struct RBItrStack));

	_Itr->Node = _Node;
	_Itr->Prev = _Prev;
	return _Itr;
}

int RBIterate(struct RBTree* _Tree, int(*_Callback)(void*)) {
	struct RBItrStack* _Stack = NULL;
	struct RBItrStack* _Temp = NULL;
	struct RBItrStack* _Delete = NULL;
	struct RBNode* _Itr = NULL;

	if(_Tree->Table == NULL)
		return 0;
	_Stack = RBStackPush(NULL, NULL);
	_Delete = RBStackPush(NULL, NULL);
	_Itr = _Tree->Table;

	while(_Itr != NULL) {
		if(_Callback(_Itr->Data) == 1)
			_Delete = RBStackPush(_Delete, _Itr);
		if(_Itr->Right != NULL)
			_Stack = RBStackPush(_Stack, _Itr->Right);
		if(_Itr->Left != NULL)
			_Stack = RBStackPush(_Stack, _Itr->Left);
		_Temp = _Stack;
		_Itr = _Stack->Node;
		_Stack = _Stack->Prev;
		free(_Temp);
	}
	if(_Delete->Node == NULL) {
		free(_Delete);
		return 0;
	}
	_Itr = _Delete->Node;
	while(_Itr != NULL) {
		RBDeleteNode(_Tree, _Delete->Node);
		_Temp = _Delete;
		_Itr = _Delete->Node;
		_Delete = _Delete->Prev;
		free(_Temp);
	}
	return 0;
}

void RBRemoveAll(struct RBTree* _Tree, void(*_Callback)(void*)) {
	struct RBItrStack* _Stack = NULL;
	struct RBItrStack* _Temp = NULL;
	struct RBNode* _Itr = NULL;

	if(_Tree->Table == NULL)
		return;
	_Stack = RBStackPush(NULL, NULL);
	_Itr = _Tree->Table;

	while(_Itr != NULL) {
		_Callback(_Itr->Data);
		if(_Itr->Right != NULL)
			_Stack = RBStackPush(_Stack, _Itr->Right);
		if(_Itr->Left != NULL)
			_Stack = RBStackPush(_Stack, _Itr->Left);
		_Temp = _Stack;
		_Itr = _Stack->Node;
		_Stack = _Stack->Prev;
		free(_Temp);
	}

	while(_Tree->Size > 0) {
		RBDeleteNode(_Tree, _Tree->Table);
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
	return RBColorCheck_Aux(_Node->Left) == RBColorCheck_Aux(_Node->Right) + _Node->Color;
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
		_Buffer += RB_STRNULL + RB_STRDELM;
		return RB_STRNULL + RB_STRDELM;
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
