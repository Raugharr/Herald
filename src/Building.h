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

enum {
	ERES_HUMAN = (1 << 0),
	ERES_ANIMAL = (1 << 1)
};

struct Building {
	int Id;
	int Width;
	int Length;
	int ResidentType;
	char* Name;
	struct Array* OutputGoods; //Contains InputReq*.
	struct Array* BuildMats; //Contains InputReq*.
};

struct Building* CreateBuilding(const char* _Name, int _Width, int _Length, int _ResType);
struct Building* CopyBuilding(const struct Building* _Building);
void DestroyBuilding(struct Building* _Building);
//! Returns the number of OutputGood that are made.
int BuildingProduce(const struct Building* _Building, struct HashTable* _Hash);
struct Building* BuildingLoad(lua_State* _State, int _Index);

#endif
