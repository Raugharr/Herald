/*
 * File: LinkedList.h
 * Author: David Brotz
 */

#ifndef __LINKEDLIST_H
#define __LINKEDLIST_H

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
void LnkLst_PushBack(struct LinkedList* _List, void* _Value);
void LnkLst_PopFront(struct LinkedList* _List);
void LnkLst_Remove(struct LinkedList* _List, struct LnkLst_Node* _Prev, struct LnkLst_Node* _Node);
#endif
