/*
 * Author: David Brotz
 * File: Int.c 
*/

#include "ITree.h"

#include <stdlib.h>

struct IntTreeNode* CreateIntTreeNode(struct IntTreeNode* _Parent, uint32_t _Key, void* _Value, int _Color) {
	struct IntTreeNode* _Node = (struct IntTreeNode*) malloc(sizeof(struct IntTreeNode));

	_Node->Node.Color = _Color;
	_Node->Node.Data = _Value;
	_Node->Node.Parent = &_Parent->Node;
	_Node->Node.Left = NULL;
	_Node->Node.Right = NULL;
	_Node->Key = _Key;
	return _Node;
}

struct IntTree* CreateITree() {
	struct IntTree* _Tree = (struct IntTree*) malloc(sizeof(struct IntTree));

	_Tree->Table = NULL;
	_Tree->Size = 0;
	return _Tree;
}

void DestroyInt(struct IntTree* _Tree) {

	free(_Tree);
}

void IntInsert(struct IntTree* _Tree, uint32_t _Key, void* _Value) {
	struct IntTreeNode* _Itr = NULL;
	struct IntTreeNode* _Node = NULL;

	if(_Value == NULL)
		return;
	if(_Tree->Size == 0) {
		_Tree->Table = CreateIntTreeNode(NULL, _Key, _Value, RB_BLACK);
		++_Tree->Size;
		return;
	}
	_Itr = _Tree->Table;
	while(_Itr != NULL) {
		if(_Key < _Itr->Key)
			if(_Itr->Node.Left == NULL) {
				_Node = CreateIntTreeNode(_Itr, _Key, _Value, RB_RED);
				_Itr->Node.Left = &_Node->Node;
				break;
			} else
			_Itr = (struct IntTreeNode*) _Itr->Node.Left;
		else
			if(_Itr->Node.Right == NULL) {
				_Node = CreateIntTreeNode(_Itr, _Key, _Value, RB_RED);
				_Itr->Node.Right = &_Node->Node;
				break;
			} else
			_Itr = (struct IntTreeNode*) _Itr->Node.Right;
	}
	RBBalance((struct RBTree*)_Tree, &_Node->Node);
	++_Tree->Size;
}

void* IntSearch(const struct IntTree* _Tree, uint32_t _Key) {
	struct IntTreeNode* _Node = IntSearchNode(_Tree, _Key);

	if(_Node != NULL)
		return _Node->Node.Data;
	return NULL;
}

struct IntTreeNode* IntSearchNode(const struct IntTree* _Tree, uint32_t _Key) {
	struct IntTreeNode* _Node = NULL;

	_Node = _Tree->Table;
	while(_Node != NULL) {
		if(_Node->Key == _Key)
			return _Node;
		else if(_Key < _Node->Key)
			_Node = (struct IntTreeNode*) _Node->Node.Left;
		else
			_Node = (struct IntTreeNode*) _Node->Node.Right;
	}
	return NULL;
}

void IntDelete(struct IntTree* _Tree, uint32_t _Key) {
	IntDeleteNode(_Tree, IntSearchNode(_Tree, _Key));
}

void IntDeleteNode(struct IntTree* _Tree, struct IntTreeNode* _OldNode) {
	struct RBNode* _Node = NULL;
	struct RBNode* _NewNode = NULL;
	struct RBNode* _Parent = NULL;
	struct RBNode* _Sibling = NULL;

	if(_OldNode == NULL)
		return;

	if(_Tree->Size == 1) {
		free(_OldNode);
		_Tree->Table = NULL;
		_Tree->Size = 0;
		return;
	}

	/*if(_OldNode->Node.Left != NULL) {
		if(_OldNode->Node.Left->Right != NULL) {
			_NewNode = __RBMax(_OldNode->Node.Left->Right);
		} else { 
			_NewNode = _OldNode->Node.Left;
		}
	} else if(_OldNode->Node.Right != NULL) {
		if(_OldNode->Node.Right->Left != NULL) {
			_NewNode = __RBMin(_OldNode->Node.Right->Left);
		} else {
			_NewNode = _OldNode->Node.Right;
		}
	} else {
		_NewNode = _OldNode;
		goto skip_loop;	
	} */

	_Node = _NewNode;
	_Node->Color = ((_Node->Color == RB_RED) ? (RB_BLACK) : (RB_DBLACK));
	while(&_Tree->Table->Node != _Node && _Node->Color == RB_DBLACK) {
		_Sibling = RBSibling(_Node);
		_Parent = _Node->Parent;
		if(_Sibling != NULL && _Sibling->Color == RB_BLACK) {
			if(_Sibling->Left != NULL && _Sibling->Left->Color == RB_RED) {
				RBRotateRight(((struct RBNode**)&_Tree->Table), _Sibling);
				RBRotateLeft(((struct RBNode**)&_Tree->Table), _Sibling->Parent);
				_Node->Color = RB_BLACK;
			} else if(_Sibling->Right != NULL && _Sibling->Right->Color == RB_RED) {
				_Sibling->Right->Color = RB_BLACK;
				RBRotateLeft(((struct RBNode**)&_Tree->Table), _Sibling);
				_Node->Color = RB_BLACK;
			}
		} else if(_Sibling != NULL && _Sibling->Color == RB_RED) {
			_Sibling->Color = RB_BLACK;
			_Parent->Color = RB_RED;
			if(_Parent->Right == _Sibling)
				RBRotateRight(((struct RBNode**)&_Tree->Table), _Sibling);
			else
				RBRotateLeft(((struct RBNode**)&_Tree->Table), _Sibling);
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
	_Tree->Table->Node.Color = RB_BLACK;
	--_Tree->Size;
	_OldNode->Node.Data = _NewNode->Data;
	_OldNode->Key = ((struct IntTreeNode*)_NewNode)->Key;
	free(_NewNode);
}
