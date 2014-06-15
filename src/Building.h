/*
 * File: Building.h
 * Author: David Brotz
 */

#ifndef __BUILDING_H
#define __BUILDING_H

#include "sys/LinkedList.h"

typedef struct lua_State lua_State;
struct Good;
struct HashTable;

struct Building {
	char* Name;
	struct Good* OutputGood;
	int Throughput;
	int Tax;//Property tax on this building.
	int Size; //Size in square feet.
	int Id;
	struct LinkedList BuildMats;//BuildMaterials that are required to build this building.
	struct LinkedList Animals;
};

struct Building* CreateBuilding(const char* _Name, struct Good* _Output, int _Tax, int _Throughput, int _SquareFeet);
struct Building* CopyBuilding(const struct Building* _Building, struct Good* _Good);
void DestroyBuilding(struct Building* _Building);
//! Returns the number of OutputGood that are made.
int Building_Produce(const struct Building* _Building, struct HashTable* _Hash);
struct Building* BuildingLoad(lua_State* _State, int _Index);

#endif
