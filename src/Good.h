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
	ESEED = (1 << 3),
	ETOOL = (1 << 4),
	EMATERIAL = (1 << 5),
	ECLOTHING = (1 << 6),
	EOTHER = (1 << 7)
};

enum {
	ETOOL_PLOW = (1 << 0),
	ETOOL_REAP = (1 << 1)
};

struct GoodBase {
	int Id;
	char* Name;
	int Category;
	struct InputReq** InputGoods;
	int IGSize;
};

struct Good {
	const struct GoodBase* Base;
	int Quantity; //!Described either as fluid ounces, ounces, or per item.
};

struct Tool {
	struct GoodBase* Base;
	int Quantity;
	int Function;
};

/*!
 * @Brief struct that contains a list of each Good that is required as well
 * as if the Good is obtainable.
 */
struct GoodDep {
	struct Array* DepTbl; //Contains struct InputReq*.
	const struct GoodBase* Good;
};

struct GoodBase* CreateGoodBase(const char* _Name, int _Category);
struct GoodBase* CopyGoodBase(const struct GoodBase* _Good);
void DestroyGoodBase(struct GoodBase* _Good);

struct Good* CreateGood(struct GoodBase* _Base);
void DestroyGood(struct Good* _Good);

struct Tool* CreateTool(struct GoodBase* _Base, int _Function);
void DestroyTool(struct Tool* _Tool);
/*!
 * @Brief Reads a table from _Index from _State that contains data about a Good.
 * @Return NULL if the table is invalid.
 * @Return Good* if the table is valid.
 */
struct GoodBase* GoodLoad(lua_State* _State, int _Index);
int GoodLoadInput(lua_State* _State, int _Index, struct GoodBase* _Good);
struct GoodDep* CreateGoodDep(const struct GoodBase* _Good);
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
struct GoodDep* GoodDependencies(struct RBTree* _Tree, const struct GoodBase* _Good);

#endif
