/*
 * File: RBTree.c
 * Author: David Brotz
 */

#include "RBTree.h"

#include "Log.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define RBSTRNULL (4) //size of "NULL".
#define RBCONTR (RBSTRPTR + RBSTRCLR + 2)
#define RBSTRCLR (2)
#define RBSTRPTR (8) //size of pointer in style of, FFFFFFFF.
#define RBSTRDELM (1)
#define RBDELM " "
#define RBNodeSwap(Copy, Node)			\
	(Copy)->Parent = (Node)->Parent;		\
	(Copy)->Left = (Node)->Left;			\
	(Copy)->Right = (Node)->Right

struct RBNode* _RBMax(struct RBNode* Node) {
	if(Node == NULL)
		return NULL;
	//if(Node->Left == NULL)
	//	return Node;
	//Node = Node->Left;
	while(Node->Right != NULL) {
		Node = Node->Right;
	}
	return Node;
}

struct RBNode* _RBMin(struct RBNode* Node) {
	if(Node == NULL)
		return NULL;
	while(Node->Left != NULL) {
		Node = Node->Left;
	}
	return Node;
}

static inline void RBReplace(struct RBNode* Node, struct RBNode* Rep) {
	Rep->Parent = Node->Parent;
	Rep->Left = Node->Left;
	Rep->Right = Node->Right;
}

void RBRotateLeft(struct RBNode** Tree, struct RBNode* Root) {
	struct RBNode* Parent = Root->Parent;
	struct RBNode* Pivot = Root->Right;

	if(Pivot == NULL)
			return;
	Root->Right = Pivot->Left;
	if(Pivot->Left != NULL)
		Pivot->Left->Parent = Root;
	Pivot->Parent = Root->Parent;
	if(Root->Parent == NULL)
		(*Tree) = Pivot;
	else
		if(Root == Parent->Left)
			Parent->Left = Pivot;
		else
			Parent->Right = Pivot;
	Pivot->Left = Root;
	Root->Parent = Pivot;
}

void RBRotateRight(struct RBNode** Tree, struct RBNode* Root) {
	struct RBNode* Parent = Root->Parent;
	struct RBNode* Pivot = Root->Left;

	if(Pivot == NULL)
		return;
	Root->Left = Pivot->Right;
	if(Pivot->Right != NULL)
		Pivot->Right->Parent = Root;
	Pivot->Parent = Root->Parent;
	if(Root->Parent == NULL)
		(*Tree) = Pivot;
	else
		if(Root == Parent->Right)
			Parent->Right = Pivot;
		else
			Parent->Left = Pivot;
	Pivot->Right = Root;
	Root->Parent = Pivot;
}

struct RBNode* RBGrandparent(struct RBNode* Node) {
	if(Node != NULL && Node->Parent != NULL)
		return Node->Parent->Parent;
	return NULL;
}

struct RBNode* RBUncle(struct RBNode* Node) {
	struct RBNode* Grand = RBGrandparent(Node);

	if(Grand == NULL)
		return Grand;
	if(Node->Parent == Grand->Left)
		return Grand->Right;
	return Grand->Left;
}

struct RBNode* CreateRBNode(struct RBNode* Parent, void* Data, int Color) {
	struct RBNode* Node = (struct RBNode*) malloc(sizeof(struct RBNode));

	Node->Color = Color;
	Node->Data = Data;
	Node->Parent = Parent;
	Node->Left = NULL;
	Node->Right = NULL;
	return Node;
}

struct RBNode* CopyRBNode(struct RBNode* Node) {
	struct RBNode* NewNode = (struct RBNode*) malloc(sizeof(struct RBNode));

	NewNode->Color = Node->Color;
	NewNode->Data = Node->Data;
	NewNode->Parent = NULL;
	if(Node->Left) {
		NewNode->Left = CopyRBNode(Node->Left);
		NewNode->Left->Parent = NewNode;
	} else
		NewNode->Left = NULL;

	if(Node->Right) {
		NewNode->Right = CopyRBNode(Node->Right);
		NewNode->Right->Parent = NewNode;
	} else
		NewNode->Right = NULL;

	return NewNode;
}

struct RBTree* CreateRBTree(RBCallback ICallBack, RBCallback SCallBack) {
	struct RBTree* Tree = (struct RBTree*) malloc(sizeof(struct RBTree));

	Tree->Table = NULL;
	Tree->Size = 0;
	Tree->ICallback = ICallBack;
	Tree->SCallback = SCallBack;
	return Tree;
}

struct RBTree* CopyRBTree(struct RBTree* Tree) {
	struct RBTree* NewTree = (struct RBTree*) malloc(sizeof(struct RBTree));

	if(Tree->Table != NULL)
		NewTree->Table = CopyRBNode(Tree->Table);
	else
		NewTree->Table = NULL;
	NewTree->Size = Tree->Size;
	NewTree->ICallback = Tree->ICallback;
	NewTree->SCallback = Tree->SCallback;
	return NewTree;
}

void DestroyRBTree(struct RBTree* Tree) {
	RBClear(Tree);
	free(Tree);
}

void RBBalance(struct RBTree* Tree, struct RBNode* Node) {
	struct RBNode* Parent = NULL;
	struct RBNode* Uncle = NULL;

	while(Node != Tree->Table && Node->Parent->Color == RB_RED) {
		Parent = Node->Parent;
		if(Parent->Parent == NULL)
			break;
		if(Parent == Parent->Parent->Left) {
			Uncle = Parent->Parent->Right;
			if(Uncle != NULL && Uncle->Color == RB_RED) {
				Parent->Color = RB_BLACK;
				Uncle->Color = RB_BLACK;
				Parent->Parent->Color = RB_RED;
				Node = Parent->Parent;
			} else {
				if(Node == Parent->Right) {
					Node = Parent;
					RBRotateLeft(&Tree->Table, Node);
					Parent = Node->Parent;
				}
				Parent->Color = RB_BLACK;
				Parent->Parent->Color = RB_RED;
				RBRotateRight(&Tree->Table, Parent->Parent);
			}
			//FIXME: Parent can only be Parent->Parent->Right or Parent->Parent->Left.
			//If it is not Parent->Parent->Left it must be its right.
		} else {//if(Parent == Parent->Parent->Right) {
			Uncle = Parent->Parent->Left;
			if(Uncle != NULL && Uncle->Color == RB_RED) {
				Parent->Color = RB_BLACK;
				Uncle->Color = RB_BLACK;
				Parent->Parent->Color = RB_RED;
				Node = Parent->Parent;
			} else {
				if(Node == Parent->Left) {
					Node = Parent;
					RBRotateRight(&Tree->Table, Node);
					Parent = Node->Parent;
				}
				Parent->Color = RB_BLACK;
				Parent->Parent->Color = RB_RED;
				RBRotateLeft(&Tree->Table, Parent->Parent);
			}
		}
	}
	Tree->Table->Color = RB_BLACK;
}

void RBInsert(struct RBTree* Tree, void* Data) {
	struct RBNode* Itr = NULL;
	struct RBNode* Node = NULL;

	if(Data == NULL)
		return;
	if(Tree->Size == 0) {
		Tree->Table = CreateRBNode(NULL, Data, RB_BLACK);
		++Tree->Size;
		return;
	}
	Itr = Tree->Table;
	while(Itr != NULL) {
		if(Tree->ICallback(Data, Itr->Data) < 0)
			if(Itr->Left == NULL) {
				Node = CreateRBNode(Itr, Data, RB_RED);
				Itr->Left = Node;
				break;
			} else
			Itr = Itr->Left;
		else
			if(Itr->Right == NULL) {
				Node = CreateRBNode(Itr, Data, RB_RED);
				Itr->Right = Node;
				break;
			} else
			Itr = Itr->Right;
	}
	RBBalance(Tree, Node);
	++Tree->Size;
}

struct RBNode* RBInsertSearch(struct RBTree* Tree, void* Search, void* Insert) {
	struct RBNode* Itr = NULL;
	struct RBNode* Node = NULL;
	int Found;

	if(Search == NULL || Insert == NULL)
		return 0;
	if(Tree->Size == 0) {
		Tree->Table = CreateRBNode(NULL, Insert, RB_BLACK);
		++Tree->Size;
		return NULL;
	}
	Itr = Tree->Table;
	while(Itr != NULL) {
		Found = Tree->ICallback(Search, Itr->Data);
		if(Found < 0)
			if(Itr->Left == NULL) {
				Node = CreateRBNode(Itr, Insert, RB_RED);
				Itr->Left = Node;
				break;
			} else
			Itr = Itr->Left;
		else if(Found > 0)
			if(Itr->Right == NULL) {
				Node = CreateRBNode(Itr, Insert, RB_RED);
				Itr->Right = Node;
				break;
			} else
				Itr = Itr->Right;
		else
			return Itr;
	}
	RBBalance(Tree, Node);
	++Tree->Size;
	return NULL;
}

void* RBSearch(const struct RBTree* Tree, const void* Data) {
	struct RBNode* Node = RBSearchNode(Tree, Data);

	if(Node != NULL)
		return Node->Data;
	return NULL;
}

struct RBNode* RBSearchNode(const struct RBTree* Tree, const void* Data) {
	struct RBNode* Node = NULL;
	int Cmp = 0;

	if(Data == NULL)
		return NULL;
	Node = Tree->Table;
	while(Node != NULL) {
		Cmp = Tree->SCallback(Data, Node->Data);
		if(Cmp == 0)
			return Node;
		else if(Cmp < 0)
			Node = Node->Left;
		else
			Node = Node->Right;
	}
		return NULL;
}

void RBDelete(struct RBTree* Tree, void* Data) {
	void* Node = RBSearchNode(Tree, Data);

	Assert(Node != NULL);
	RBDeleteNode(Tree, Node);
}

/**
 * It is possible to just move the data field from Node to OldNode
 * but we want to be able to ensure that pointers to RBNode*s will not
 * be invalidated.
 */
/*void RBDeleteNode(struct RBTree* Tree, struct RBNode* OldNode) {
	struct RBNode* Node = NULL;
	struct RBNode* NewNode = NULL;
	struct RBNode* Parent = NULL;
	struct RBNode* Sibling = NULL;

	if(OldNode == NULL)
		return;

	if(Tree->Size == 1) {
		#ifdef DEBUG
				assert(Tree->Table->Left == NULL && Tree->Table->Right == NULL);
		#endif
		free(OldNode);
		Tree->Table = NULL;
		Tree->Size = 0;
		return;
	}

	if(OldNode->Left != NULL) {
		if(OldNode->Left->Right != NULL) {
			NewNode = _RBMax(OldNode->Left->Right);
		} else { 
			NewNode = OldNode->Left;
		}
	} else if(OldNode->Right != NULL) {
		if(OldNode->Right->Left != NULL) {
			NewNode = _RBMin(OldNode->Right->Left);
		} else {
			NewNode = OldNode->Right;
		}
	} else {
		NewNode = OldNode;
		goto skip_loop;	
	}

	Node = NewNode;
	Node->Color = ((Node->Color == RB_RED) ? (RB_BLACK) : (RB_DBLACK));
	while(Tree->Table != Node && Node->Color == RB_DBLACK) {
		Sibling = RBSibling(Node);
		Parent = Node->Parent;
		if(Sibling != NULL && Sibling->Color == RB_BLACK) {
			if(Sibling->Left != NULL && Sibling->Left->Color == RB_RED) {
				RBRotateRight(&Tree->Table, Sibling);
				RBRotateLeft(&Tree->Table, Sibling->Parent);
				Node->Color = RB_BLACK;
			} else if(Sibling->Right != NULL && Sibling->Right->Color == RB_RED) {
				Sibling->Right->Color = RB_BLACK;
				RBRotateLeft(&Tree->Table, Sibling);
				Node->Color = RB_BLACK;
			}
		} else if(Sibling != NULL && Sibling->Color == RB_RED) {
			Sibling->Color = RB_BLACK;
			Parent->Color = RB_RED;
			if(Parent->Right == Sibling)
				RBRotateRight(&Tree->Table, Sibling);
			else
				RBRotateLeft(&Tree->Table, Sibling);
			continue;
		}
		if(Node->Parent != NULL) {
			if(Parent->Color == RB_RED) {
				Node->Color = RB_BLACK;
				Parent->Color = RB_BLACK;
				if(Sibling != NULL) {
					Sibling->Color = RB_RED;
				} else {
					Parent->Color = RB_DBLACK;
					Node = Parent;
				}
			} else {
				Node->Color = RB_BLACK;
				Parent->Color = RB_DBLACK;
				if(Sibling != NULL)
					Sibling->Color = RB_RED;
				Node = Parent;
				continue;
			}
		}
	}
	skip_loop:
	Parent = NewNode->Parent;
	if(Parent) {
		struct RBNode* Temp = (NewNode->Left == NULL) ? (NewNode->Right) : (NewNode->Left);

		if(Parent->Right == NewNode)
			Parent->Right = Temp;
		else
			Parent->Left = Temp;
		if(Temp)
			Temp->Parent = Parent;
	}
	Tree->Table->Color = RB_BLACK;
	--Tree->Size;
	OldNode->Data = NewNode->Data;
	free(NewNode);
}*/

struct RBNode* RBSucessor(struct RBNode* Root, struct RBNode* Node) {
	struct RBNode* Swap = NULL;

	Swap = Node->Right;
	if(Swap != NULL) {
		while(Swap->Left != NULL) Swap = Swap->Left;
		return Swap;
	} else {
		Swap = Node->Parent;
		while(Swap != NULL && Node == Swap->Right) {
			Node = Swap;
			Swap = Swap->Parent;
		}
	}
	return (Swap == Root) ? (NULL) : (Swap);
}

void RBDelBalance(struct RBTree* Tree, struct RBNode* Node) {
	struct RBNode* Sibling = NULL;

/*	if(Node->Left != NULL && Node->Right == NULL) {
		struct RBNode* Child = Node->Left;

		Child->Parent = Node->Parent;
		if(Node->Parent->Left == Node) Node->Parent->Left = Child;
		else Node->Parent->Right = Child;
		if(Node->Color == RB_BLACK && Child->Color == RB_RED) {
			Child->Color = RB_BLACK;
			return;
		}
	} else if(Node->Left == NULL && Node->Right != NULL) {
		struct RBNode* Child = Node->Right;

		Child->Parent = Node->Parent;
		if(Node->Parent->Left == Node) Node->Parent->Left = Child;
		else Node->Parent->Right = Child;
		if(Node->Color == RB_BLACK && Child->Color == RB_RED) {
			Child->Color = RB_BLACK;
			return;
		}
	}
*/			
	while(Node != Tree->Table && Node->Color == RB_BLACK) {
		if(Node == Node->Parent->Left) {
			Sibling = Node->Parent->Right;
			if(Sibling == NULL) break;
			if(Sibling->Color == RB_RED) {
				Node->Parent->Color = RB_RED;
				Sibling->Color = RB_BLACK;
				RBRotateLeft(&Tree->Table, Node->Parent);
				Sibling = Node->Parent->Right;
				if(Sibling == NULL) break;
			}
			if((Sibling->Left == NULL || Sibling->Left->Color == RB_BLACK) && (Sibling->Right == NULL || Sibling->Right->Color == RB_BLACK)) {
				Sibling->Color = RB_RED;
				Node = Node->Parent;
			} else if(Sibling->Right != NULL && Sibling->Right->Color == RB_BLACK) {
				if(Sibling->Left != NULL) Sibling->Left->Color = RB_BLACK;
				RBRotateRight(&Tree->Table, Sibling);
				Sibling = Sibling->Parent;
				if(Sibling == NULL) break;
			}
			if(Node->Parent == NULL) break;
			Sibling->Color = Node->Parent->Color;
			Node->Parent->Color = RB_BLACK;
			if(Sibling->Right != NULL) Sibling->Right->Color = RB_BLACK;
			RBRotateLeft(&Tree->Table, Node->Parent);
			break;
		} else {
			Sibling = Node->Parent->Left;
			if(Sibling == NULL) break;
			if(Sibling->Color == RB_RED) {
				Node->Parent->Color = RB_RED;
				Sibling->Color = RB_BLACK;
				RBRotateRight(&Tree->Table, Node->Parent);
				Sibling = Node->Parent->Left;
				if(Sibling == NULL) break;
			}
			if((Sibling->Left == NULL || Sibling->Left->Color == RB_BLACK) && (Sibling->Right == NULL || Sibling->Right->Color == RB_BLACK)) {
				Sibling->Color = RB_RED;
				Node = Node->Parent;
			} else if(Sibling->Left != NULL && Sibling->Left->Color == RB_BLACK) {
				if(Sibling->Right != NULL) Sibling->Right->Color = RB_BLACK;
				RBRotateLeft(&Tree->Table, Sibling);
				Sibling = Sibling->Parent;
				if(Sibling == NULL) break;
			}
			if(Node->Parent == NULL) break;
			Sibling->Color = Node->Parent->Color;
			Node->Parent->Color = RB_BLACK;
			if(Sibling->Left != NULL) Sibling->Left->Color = RB_BLACK;
			RBRotateRight(&Tree->Table, Node->Parent);
			break;
		}
	}
	Node->Color = RB_BLACK;
}

void RBDeleteNode(struct RBTree* Tree, struct RBNode* OldNode) {
	struct RBNode* Node = NULL;
	struct RBNode* Sibling = NULL;

	Node = ((OldNode->Left == NULL || OldNode->Right == NULL)) ? (OldNode) : (RBSucessor(Tree->Table, OldNode));
	Sibling = (Node->Left == NULL) ? (Node->Right) : (Node->Left);
	if(Sibling != NULL) Sibling->Parent = Node->Parent;
	if(Node->Parent == NULL) {
		Tree->Table = Sibling;
	} else {
		if(Node == Node->Parent->Left)
			Node->Parent->Left = Sibling;
		else
			Node->Parent->Right = Sibling;
	}
	if(Node != OldNode)
		OldNode->Data = Node->Data;
	if(Node->Color == RB_BLACK && Sibling != NULL)
		RBDelBalance(Tree, Sibling);
	free(Node);
	--Tree->Size;
	/*if(Tree->Table == Sibling->Parent) {
		Tree->Table->Left = Sibling;
	} else {
		if(Node == Node->Parent->Left)
			Node->Parent->Left = Sibling;
		else
			Node->Parent->Right = Sibling;
	}*/

}
/*
 * FIXME: When the RBTree contains thousands of elements it would take to long to iterate through the thousands
 * of elements put them into a stack and then give them to the caller. Instead we should do something like the
 * commented code below where we use assembly to pass a variable amount of arguments to a callback function.
 */
struct RBItrStack* RBDepthFirst(struct RBNode* const Node, struct RBItrStack* Stack) {
	if(Node == NULL)
		return Stack;
	(*Stack).Prev = Stack;
	(*Stack).Node = Node;
	++Stack;
	Stack = RBDepthFirst(Node->Left, Stack);
	Stack = RBDepthFirst(Node->Right, Stack);
	return Stack;
}

/*
 * void RBDepthFirst(struct RBNode* const Node, void(*Callback)(), void** Args, int ArgSize) {
 * int i = 0;
	if(Node == NULL)
		return;
	_asm__("pushl $_Args[i]\n\t
			addl $1, $i\n\t
			call Callback\n\t
			");
	Stack = RBDepthFirst(Node->Left, Callback, Args, ArgSize);
	Stack = RBDepthFirst(Node->Right Callback, Args, ArgSize);
}
 */

void RBIterate(struct RBTree* Tree, int(*Callback)(void*)) {
	int i = 0;
	int j = 0;
	struct RBItrStack Stack[Tree->Size];
	struct RBItrStack DeleteStack[Tree->Size];

	if(Tree->Table == NULL)
		return;
	memset(Stack, 0, sizeof(struct RBItrStack*) * Tree->Size);
	memset(DeleteStack, 0, sizeof(struct RBItrStack*) * Tree->Size);
	RBDepthFirst(Tree->Table, Stack);
	for(i = 0; i < Tree->Size; ++i) {
		if(Callback(Stack[i].Node->Data) != 0)
			DeleteStack[j] = Stack[i];
	}
	for(i = 0; i < j; ++i)
		RBDeleteNode(Tree, DeleteStack[i].Node);
}

void RBRemoveAll(struct RBTree* Tree, void(*Callback)(void*)) {
	struct RBItrStack ItrStack[Tree->Size];
	int i = 0;

	memset(ItrStack, 0, sizeof(struct RBItrStack*) * Tree->Size);
	RBDepthFirst(Tree->Table, ItrStack);
	for(i = 0; i < Tree->Size; ++i) {
		Callback(ItrStack[i].Node->Data);
		RBDeleteNode(Tree, Tree->Table);
	}
}

void RBClear(struct RBTree* Tree) {
	while(Tree->Size > 0) {
		RBDeleteNode(Tree, Tree->Table);
	}
}

void* RBMax(struct RBNode* Node) {
	return _RBMax(Node)->Data;
}
void* RBMin(struct RBNode* Node) {
	return _RBMin(Node)->Data;
}

int RBHeight_Aux(struct RBNode* Node) {
	if(Node == NULL)
		return 0;
	return RBHeight_Aux(Node->Left) + RBHeight_Aux(Node->Right) + 1;
}

int RBInvariant(struct RBNode* Node, RBCallback ICallback) {
	int Correct = 1;
	if(Node == NULL)
		return 1;
	if(Node->Left != NULL) {
		Correct = Correct && (ICallback(Node->Data, Node->Left->Data) > 0) && RBInvariant(Node->Left, ICallback);
	}
	if(Node->Right != NULL) {
		Correct = Correct && (ICallback(Node->Data, Node->Right->Data) < 0) && RBInvariant(Node->Right, ICallback);
	}
	return Correct;
}

int RBCount(struct RBNode* Node) {
	if(Node == NULL)
		return 0;
	return RBCount(Node->Left) + RBCount(Node->Right) + 1;
}

int RBHeight(struct RBNode* Node) {
	int Left = 0;
	int Right = 0;

	if(Node == NULL)
		return 0;

	Left = RBHeight_Aux(Node->Left);
	Right = RBHeight_Aux(Node->Right);
	return (Left < Right) ? (Right) : (Left);
}

int RBColorCheck_Aux(struct RBNode* Node) {
	if(Node == NULL)
		return 1;
	return RBColorCheck_Aux(Node->Left) == RBColorCheck_Aux(Node->Right) + Node->Color;
}

int RBColorCheck(struct RBNode* Node) {
	if(Node == NULL)
		return 1;
	return RBColorCheck(Node->Left) == RBColorCheck(Node->Right);
}

int RBStrlen(struct RBNode* Node) {
	if(Node == NULL)
		return RBSTRNULL + RBSTRDELM;
	return RBStrlen(Node->Left) + RBStrlen(Node->Right) + RBCONTR + RBSTRDELM;
}

int RBToString(struct RBNode* Node, char* Buffer, int Size) {
	if(Node == NULL) {
		if(Size < RBSTRNULL + RBSTRDELM + 1)
			return 0;
		strncat(Buffer, "NULL ", Size);
		Buffer += RBSTRNULL + RBSTRDELM;
		return RBSTRNULL + RBSTRDELM;
	}
	if(Size < RBCONTR + 1)
		return 0;
	char Temp[RBCONTR + 1];
	sprintf(Temp, "[%p %d]", Node, Node->Color);
	strncat(Buffer, Temp, Size);
	strcat(Buffer, RBDELM);
	Buffer += RBCONTR + RBSTRDELM;
	Size -= RBCONTR + RBSTRDELM;
	return RBToString(Node->Left, Buffer, Size) + RBToString(Node->Right, Buffer, Size);
}

int RBRange(struct RBTree* Tree, void* Min, void* Max, void** RangeTbl, int MaxSize) {
	struct RBNode* Node = NULL;
	int LowCmp = 0;
	int HighCmp = 0;
	struct RBNode* NodeList[256];
	int NodeListSz = 1;
	int RangeTblSz = 0;

	NodeList[0] = Tree->Table;
	while(NodeListSz > 0) {
		Node = NodeList[--NodeListSz];
		if((LowCmp = Tree->SCallback(Node->Data, Min)) >= 0 && (HighCmp = Tree->SCallback(Node->Data, Max)) <= 0) {
			if(RangeTblSz >= MaxSize)
				return RangeTblSz;
			RangeTbl[RangeTblSz++] = Node->Data;
			if(Node->Left != NULL)
				NodeList[NodeListSz++] = Node->Left;
			if(Node->Right !=  NULL)
				NodeList[NodeListSz++] = Node->Right; 
		} else if(LowCmp < 0) {
			if(Node->Right != NULL)
				NodeList[NodeListSz++] = Node->Right;
		} else if(HighCmp > 0) {
			if(Node->Left != NULL)
				NodeList[NodeListSz++] = Node->Left;
		}
	}
	return RangeTblSz;
}
