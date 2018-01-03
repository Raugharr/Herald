/*
 * File: Building.h
 * Author: David Brotz
 */

#ifndef __BUILDING_H
#define __BUILDING_H

#include "Herald.h"

#include <inttypes.h>

#include <SDL2/SDL.h>

typedef struct lua_State lua_State;
struct GoodBase;
struct HashTable;
struct Array;
struct Object;
struct Zone;
struct LnkLst_Node;

enum {
	ERES_HUMAN = (1 << 0),
	ERES_ANIMAL = (1 << 1)
};

enum {
	BMAT_WALL = 1,
	BMAT_FLOOR = 2,
	BMAT_ROOF = 3
};

enum {
	EBT_HOME,
	EBT_GRANARY
};

struct Building {
	struct Object Object;
	SDL_Point Pos;
	const struct BuildMat* Walls;
	const struct BuildMat* Floor;
	const struct BuildMat* Roof;
	struct InputReq** OutputGoods;
	struct InputReq** BuildMats;
	uint32_t SquareFeet;
	uint8_t ResidentType;
};

struct Construction {
	int Id;
	int Type;
	void (*Think)(struct Object*);
	int LastThink; //In game ticks.
	struct LnkLst_Node* ThinkObj;
	struct Person* Worker;
	struct Building* Building;
	int DaysLeft;
};

struct BuildMat {
	int Type;
	int BuildCost;
	double MatCost;
	const char* Name;
	const struct GoodBase* Good;
};

struct Construction* CreateConstruct(struct Building* _Building, struct Person* _Person);
void DestroyConstruct(struct Construction* _Construct);

void ConstructThink(struct Construction* _Construct);
int ConstructionTime(const struct BuildMat* _Walls, const struct BuildMat* _Floor, const struct BuildMat* _Roof, int _Area);

struct Building* CreateBuilding(int _ResType, const struct BuildMat* _Walls, const struct BuildMat* _Floor, const struct BuildMat* _Roof, int _SquareFeet);
void DestroyBuilding(struct Building* _Building);

int BuildingArea(const struct Building* _Building);

/**
 * Returns a BuildMat of type _MatType that exists in _Goods that is the most plentiful.
 */
struct BuildMat* SelectBuildMat(const struct Array* _Goods, int _MatType);
struct LnkLst_Node* BuildingLoad(lua_State* _State, int _Index);
struct GoodBase* BuildMatToGoodBase(struct BuildMat* _Mat);

#endif
