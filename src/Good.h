/*
 * File: Good.h
 * Author: David Brotz
 */

#ifndef __GOOD_H
#define __GOOD_H

enum {
	EFOOD = (1 << 0),
	EINGREDIENT = (1 << 1),
	EANIMAL = (1 << 2),
	EWEAPON = (1 << 3),
	EARMOR = (1 << 4),
	ESHIELD = (1 << 5),
	ESEED = (1 << 6),
	EOTHER = (1 << 7)
};

struct Good {
	char* Name;
	int Category;
	int Quantity;
	int Id;
	int Price;
};

struct Good* CreateGood(const char* _Name, int _Category);
struct Good* CopyGood(const struct Good* _Good);
void DestroyGood(struct Good* _Good);

#endif
