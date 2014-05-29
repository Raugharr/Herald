/*
 * File: Building.h
 * Author: David Brotz
 */

#ifndef __BUILDING_H
#define __BUILDING_H

#include "sys/LinkedList.h"

struct Good;

struct Building {
	char* Name;
	struct Good* OutputGood;
	int Price;//Price of the output good.
	int Throughput;
	int Tax;//Property tax on this building.
	int Size; //Size in square feet.
	int Id;
	struct LinkedList BuildMats;//Goods that are required to build this building.
	struct LinkedList Animals;
};

struct Building* CreateBuilding(const char* _Name, struct Good* _Output, int _Tax, int _Throughput, int _SquareFeet);
struct Building* CopyBuilding(const struct Building* _Building, struct Good* _Good);
void DestroyBuilding(struct Building* _Building);

#endif
