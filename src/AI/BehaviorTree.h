/*
 * File: BehaviorTree.h
 * Author: David Brotz
 */

#ifndef __BEHAVIORTREE_H
#define __BEHAVIORTREE_H

struct Person;
struct HashTable;

typedef int(*BHVAction)(struct Person*, struct HashTable*);

enum {
	BHV_SELECTOR = 0,
	BHV_SEQUENCE
};

//Behavior decorators.
enum {
	BHV_DNOT = 0
};

struct Behavior {
	int(*Callback)(struct Behavior*, struct Person*, void*);
	BHVAction Action;
	struct Behavior** Children;
	int Size;
	struct Behavior* Parent;
};

void DestroyBehavior(struct Behavior* _Bhv);
struct Behavior* CreateBHVComp(int _Type, struct Behavior* _Bhv, ...);
struct Behavior* CreateBHVNode(BHVAction _Action);
struct Behavior* CreateBHVD(int _Type, BHVAction _Action);

int BHVRun(struct Behavior* _Bhv, struct Person* _Person, void* _Data);

#endif
