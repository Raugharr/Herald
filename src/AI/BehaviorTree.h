/*
 * File: BehaviorTree.h
 * Author: David Brotz
 */

#ifndef __BEHAVIORTREE_H
#define __BEHAVIORTREE_H

struct Person;
struct HashTable;
struct Behavior;

typedef int(*BhvCallback)(struct Behavior*, struct Person*, void*);
typedef int(*BhvAction)(struct Person*, struct HashTable*);

struct Behavior {
	BhvCallback Callback;
	BhvAction Action;
	struct Behavior** Children;
	int Size;
};

int BhvSelector(struct Behavior* _Bhv, struct Person* _Person, void* _Data);
int BhvSequence(struct Behavior* _Bhv, struct Person* _Person, void* _Data);

int BhvNot(struct Behavior* _Bhv, struct Person* _Person, void* _Data);

struct Behavior* CreateBehavior(struct Behavior* _Parent, BhvAction _Run, int _Size, BhvCallback _Callback);
void DestroyBehavior(struct Behavior* _Bhv);
struct Behavior* CreateBhvComp(BhvCallback _Callback, struct Behavior* _Bhv, ...);
struct Behavior* CreateBhvNode(BhvAction _Action);

int BHVRun(struct Behavior* _Bhv, struct Person* _Person, void* _Data);

#endif
