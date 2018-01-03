/*
 * File: LinkedList.h
 * Author: David Brotz
 */

#ifndef __LINKEDLIST_H
#define __LINKEDLIST_H

#define LinkedList() {0, NULL, NULL}

#define IMPLICIT_LINKEDLIST(_Type)			\
	_Type* Next;							\
	_Type* Prev							

#define ILL_CREATE(_Global, _Var)			\
	if((_Global) == NULL) {					\
		(_Var)->Next = NULL;				\
	} else {								\
		(_Var)->Next = (_Global);			\
		(_Global)->Prev = (_Var);			\
	}										\
	(_Var)->Prev = NULL;					\
	(_Global) = (_Var)

#define ILL_DESTROY(_Global, _Var)				\
	if((_Global) == (_Var)) {					\
		(_Global) = (_Var)->Next;				\
	} else {									\
		if((_Var)->Prev != NULL)				\
			(_Var)->Prev->Next = (_Var)->Next;	\
		if((_Var)->Next != NULL)				\
			(_Var)->Next->Prev = (_Var)->Prev;	\
	}

struct LnkLst_Node {
	void* Data;
	struct LnkLst_Node* Next;
	struct LnkLst_Node* Prev;
};

struct LinkedList {
	int Size;
	struct LnkLst_Node* Front;
	struct LnkLst_Node* Back;
};

struct LnkLst_Node* CreateLnkLstNode(void* _Data);
struct LinkedList* CreateLinkedList();
void ConstructLinkedList(struct LinkedList* _List);
void DestroyLinkedList(struct LinkedList* _List);
void LnkLstInsertPriority(struct LinkedList* _List, void* _Value, int (*_Callback)(const void*, const void*));
void LnkLstClear(struct LinkedList* _List);
void LnkLstPushBack(struct LinkedList* _List, void* _Value);
void LnkLstPushFront(struct LinkedList* _List, void* _Value);
void* LnkLstPopFront(struct LinkedList* _List);
void* LnkLstPopBack(struct LinkedList* _List);
void LnkLstInsertAfter(struct LinkedList* _List, struct LnkLst_Node* _Node, void* _Value);
void LnkLstInsertBefore(struct LinkedList* _List, struct LnkLst_Node* _Node, void* _Value);
void LnkLstRemove(struct LinkedList* _List, struct LnkLst_Node* _Node);
void* LnkLstSearch(struct LinkedList* _List, const void* _Data, int (*_Compare)(const void*, const void*));
/*!
 * @brief adds all nodes that _Node can point to including _Node to _List and then deletes them.
 */
void LnkLstCatNode(struct LinkedList* _List, struct LnkLst_Node* _Node);
void* LnkLstRandom(struct LinkedList* _List);
void** LnkLstToList(const struct LinkedList* _List);
#endif
