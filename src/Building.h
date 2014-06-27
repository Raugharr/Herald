/*
 * File: Building.h
 * Author: David Brotz
 */

#ifndef __BUILDING_H
#define __BUILDING_H

typedef struct lua_State lua_State;
struct GoodBase;
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
	int BuildTime;
	char* Name;
	struct GoodBase* Walls;
	struct GoodBase* Floor;
	struct GoodBase* Roof;
	struct Array* OutputGoods; //Contains InputReq*.
	struct Array* BuildMats; //Contains InputReq*.
};

struct Construction {
	void* Prev;
	void* Next;
	int Type;
	struct Person* Worker;
	struct Building* Building;
	int DaysLeft;
};

struct Construction* CreateConstruct(struct Building* _Building, struct Person* _Person);
struct Construction* CopyConstruct(struct Construction* _Construct);
void DestroyConstruct(struct Construction* _Construct);

int ConstructUpdate(struct Construction* _Construct);

struct Building* CreateBuilding(const char* _Name, int _Width, int _Length, int _ResType, int _BuildTime);
struct Building* CopyBuilding(const struct Building* _Building);
void DestroyBuilding(struct Building* _Building);

//! Returns the number of OutputGood that are made.
int BuildingProduce(const struct Building* _Building, struct HashTable* _Hash);
struct Building* BuildingLoad(lua_State* _State, int _Index);

#endif
