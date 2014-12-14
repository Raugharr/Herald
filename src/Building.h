/*
 * File: Building.h
 * Author: David Brotz
 */

#ifndef __BUILDING_H
#define __BUILDING_H

typedef struct lua_State lua_State;
struct GoodBase;
struct HashTable;
struct Array;
struct Object;

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
	int Id;
	int X;
	int Y;
	int(*Think)(struct Object*);
	int ResidentType;
	int Width;
	int Length;
	const struct BuildMat* Walls;
	const struct BuildMat* Floor;
	const struct BuildMat* Roof;
	struct InputReq** OutputGoods;
	struct InputReq** BuildMats;
};

struct Construction {
	void* Prev;
	void* Next;
	int Type;
	struct Person* Worker;
	struct Building* Building;
	int DaysLeft;
};

struct BuildMat {
	int Id;
	int Type;
	int BuildCost;
	double MatCost;
	const struct GoodBase* Good;
};

struct Construction* CreateConstruct(struct Building* _Building, struct Person* _Person);
struct Construction* CopyConstruct(struct Construction* _Construct);
void DestroyConstruct(struct Construction* _Construct);

int ConstructUpdate(struct Construction* _Construct);
int ConstructionTime(const struct Building* _Building, int _Width, int _Height);

struct Building* CreateBuilding(int _ResType, int _Width, int _Length, const struct BuildMat* _Walls, const struct BuildMat* _Floor, const struct BuildMat* _Roof);
struct Building* CopyBuilding(const struct Building* _Building);
void DestroyBuilding(struct Building* _Building);

struct BuildMat* SelectBuildMat(const struct Array* _Goods, int _MatType);
struct Building* BuildingPlan(const struct Person* _Person, int _Type);
struct LnkLst_Node* BuildingLoad(lua_State* _State, int _Index);

#endif
