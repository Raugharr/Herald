/*
 * File: BehaviorTree.h
 * Author: David Brotz
 */

#ifndef __BEHAVIORTREE_H
#define __BEHAVIORTREE_H

#include "../sys/Rule.h"

#define BhvIsNode(_Bhv) ((_Bhv)->Action != NULL && (_Bhv)->Callback == NULL)
#define BhvIsDecorator(_Bhv) ((_Bhv)->Action != NULL && (_Bhv)->Callback != NULL)
#define BhvIsComposite(_Bhv) ((_Bhv)->Action == NULL && (_Bhv)->Callback != NULL)

struct Person;
struct HashTable;
struct Behavior;
struct Family;
struct Primitive;

typedef int(*BhvCallback)(const struct Behavior*, struct Family*, struct HashTable*);
typedef int(*BhvAction)(struct Family*, struct HashTable*, const struct Primitive*, int);

/*
 * NOTE: Instead of having each composite have a table of its children use a global table to prevent cache lookups slowing the BT doown.
 */

struct Behavior {
	BhvAction Action;
	BhvCallback Callback;
	struct Behavior** Children;
	int Size;
};

struct BehaviorNode {
	BhvAction Action;
	BhvCallback Callback; //Not used in BehaviorNode but to align with Behavior.
	struct Primitive* Arguments;
	int ArgSize;
};

//void InitBehaviorEngine();
//void QuitBehaviorEngine();

int BhvSelector(const struct Behavior* _Bhv, struct Family* _Family, struct HashTable* _Vars);
int BhvSequence(const struct Behavior* _Bhv, struct Family* _Family, struct HashTable* _Vars);

int BhvNot(const struct Behavior* _Bhv, struct Family* _Family, struct HashTable* _Vars);

struct Behavior* CreateBehavior(struct Behavior* _Parent, BhvAction _Run, int _Size, BhvCallback _Callback);
void DestroyBehavior(struct Behavior* _Bhv);

struct BehaviorNode* CreateBehaviorNode(BhvAction _Action, int _ArgSize);
void DestroyBehaviorNode(struct BehaviorNode* _Behavior);
struct Behavior* CreateBhvComp(BhvCallback _Callback, struct Behavior* _Bhv, ...);
struct Behavior* CreateBhvNode(BhvAction _Action);

int BHVRun(const struct Behavior* _Bhv, struct Family* _Family, struct HashTable* _Vars);

#endif
