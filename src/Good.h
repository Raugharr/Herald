/*
 * File: Good.h
 * Author: David Brotz
 */

#ifndef __GOOD_H
#define __GOOD_H

#include "sys/LinkedList.h"

#define TOPOUND(__Quantity) (__Quantity * 16);

typedef struct lua_State lua_State;
struct HashTable;

enum {
	EFOOD = (1 << 0),
	EINGREDIENT = (1 << 1),
	EANIMAL = (1 << 2),
	EWEAPON = (1 << 3),
	EARMOR = (1 << 4),
	ESHIELD = (1 << 5),
	ESEED = (1 << 6),
	ETOOL = (1 << 7),
	EMATERIAL = (1 << 8),
	EOTHER = (1 << 9)
};

struct Good {
	int Id;
	char* Name;
	int Category;
	int Quantity; //!Described either as fluid ounces, ounces, or per item.
	struct LinkedList InputGoods; //!Contains struct InputReq*.
};

/*!
 * @Brief struct that contains a list of each Good that is required as well
 * as if the Good is obtainable.
 */
struct GoodDep {
	struct Array* DepTbl;
	const struct Good* Good;
};

struct Good* CreateGood(const char* _Name, int _Category);
struct Good* CopyGood(const struct Good* _Good);
void DestroyGood(struct Good* _Good);
/*!
 * @Brief Reads a table from _Index from _State that contains data about a Good.
 * @Return NULL if the table is invalid.
 * @Return Good* if the table is valid.
 */
struct Good* GoodLoad(lua_State* _State, int _Index);
int GoodLoadInput(lua_State* _State, int _Index, struct Good* _Good);
struct GoodDep* CreateGoodDep(const struct Good* _Good);
void DestroyGoodDep(struct GoodDep* _GoodDep);
/*
 * @Brief Simple wrapper that adds the correct ICallback and SCallback to RBTRee.
 * The returned RBTree* can be deleted with DestroyRBTree as normal.
 */
struct RBTree* GoodBuildDep(const struct HashTable* _GoodList);
/*!
 * @Brief Returns an empty GoodDep*. Every GoodDep* in _Tree that is a prerequisite of _Good
 * Will have this empty GoodDep* added to their DepTbl.
 */
struct GoodDep* GoodDependencies(struct RBTree* _Tree, const struct Good* _Good);

#endif
