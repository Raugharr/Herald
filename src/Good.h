/*
 * File: Good.h
 * Author: David Brotz
 */

#ifndef __GOOD_H
#define __GOOD_H

#include "sys/LinkedList.h"

#define TOPOUND(__Quantity) (__Quantity * 16);

typedef struct lua_State lua_State;

enum {
	EFOOD = (1 << 0),
	EINGREDIENT = (1 << 1),
	EANIMAL = (1 << 2),
	EWEAPON = (1 << 3),
	EARMOR = (1 << 4),
	ESHIELD = (1 << 5),
	ESEED = (1 << 6),
	ETOOL = (1 << 7),
	EOTHER = (1 << 8)
};

struct Good {
	char* Name;
	int Category;
	int Quantity; //Described either as fluid ounces, ounces, or per item.
	int Id;
	struct LinkedList InputGoods;
};

struct Food {
	struct Good* Good;
	int Nutrition;
};

struct Tool {
	struct Good* Good;
	int Function;
};

struct Good* CreateGood(const char* _Name, int _Category);
struct Good* CopyGood(const struct Good* _Good);
void DestroyGood(struct Good* _Good);

struct Good* GoodLoad(lua_State* _State, int _Index);
int GoodLoadInput(lua_State* _State, int _Index, struct Good* _Good);

#endif
