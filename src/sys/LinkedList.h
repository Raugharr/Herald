/*
 * File: LinkedList.h
 * Author: David Brotz
 */

#ifndef __LINKEDLIST_H
#define __LINKEDLIST_H

#define LnkLstPopFront(_List) LnkLst_PopFront(_List)
#define LnkLstPushBack(_List, _Node) LnkLst_PushBack(_List, _Node)
#define LnkLstRemove(_List, _Prev, _Node) LnkLst_Remove(_List, _Prev, _Node)
#define LnkLstCatNode(_List, _Node) LnkLst_CatNode(List, _Node)

#define ILL_CREATE(_Global, _Var)			\
	if(_Global == NULL)						\
		_Var->Next = NULL;					\
	else {									\
		_Var->Next = _Global;				\
		_Global->Prev = _Var;				\
	}										\
	_Var->Prev = NULL;

#define ILL_DESTROY(_Global, _Var)			\
	if(_Global == _Var) {					\
		_Global = _Var->Next;				\
	} else {								\
		if(_Var->Prev != NULL)				\
			_Var->Prev->Next = _Var->Next;	\
		if(_Var->Next != NULL)				\
			_Var->Next->Prev = _Var->Prev;	\
	}										\

struct LnkLst_Node {
	void* Data;
	struct LnkLst_Node* Next;
};

struct LinkedList {
	int Size;
	struct LnkLst_Node* Front;
	struct LnkLst_Node* Back;
};

struct LinkedList* CreateLinkedList();
void DestroyLinkedList(struct LinkedList* _List);
void LnkLstInsertPriority(struct LinkedList* _List, void* _Value, int (*_Callback)(const void*, const void*));
void LnkLst_PushBack(struct LinkedList* _List, void* _Value);
void LnkLst_PopFront(struct LinkedList* _List);
void LnkLst_Remove(struct LinkedList* _List, struct LnkLst_Node* _Prev, struct LnkLst_Node* _Node);
void* LnkLstSearch(struct LinkedList* _List, const void* _Data, int (*_Compare)(const void*, const void*));
/*!
 * @brief adds all nodes that _Node can point to including _Node to _List and then deletes them.
 */
void LnkLst_CatNode(struct LinkedList* _List, struct LnkLst_Node* _Node);
#endif
