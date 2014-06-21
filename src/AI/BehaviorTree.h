/*
 * File: BehaviorTree.h
 * Author: David Brotz
 */

#ifndef __BEHAVIORTREE_H
#define __BEHAVIORTREE_H

struct Person;

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
	int(*Action)(struct Person*, void*);
	struct Behavior** Children;
	int Size;
	struct Behavior* Parent;
};

void DestroyBehavior(struct Behavior* _Bhv);
struct Behavior* CreateBHVComp(struct Behavior* _Parent, int _Type, int _Size);
struct Behavior* CreateBHVNode(struct Behavior* _Parent, int(*_Action)(struct Person*, void*));
struct Behavior* CreateBHVD(struct Behavior* _Parent, int _Type, int(*_Action)(struct Person*, void*));

int BHVRun(struct Behavior* _Bhv, struct Person* _Person, void* _Data);

#endif
